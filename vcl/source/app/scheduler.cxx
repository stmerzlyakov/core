/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <svdata.hxx>
#include <tools/time.hxx>
#include <vcl/scheduler.hxx>
#include <vcl/timer.hxx>
#include <saltimer.hxx>

void ImplSchedulerData::Invoke()
{
    assert( mpScheduler && !mbInScheduler );
    if ( !mpScheduler || mbInScheduler )
        return;

    Scheduler *sched = mpScheduler;

    // prepare Scheduler Object for deletion after handling
    mpScheduler->SetDeletionFlags();

    // invoke it
    mbInScheduler = true;
    sched->Invoke();
    mbInScheduler = false;
}

void Scheduler::SetDeletionFlags()
{
    assert( mpSchedulerData );
    mpSchedulerData->mpScheduler = NULL;
    mpSchedulerData = NULL;
}

void Scheduler::ImplDeInitScheduler()
{
    ImplSVData*     pSVData = ImplGetSVData();
    if (pSVData->mpSalTimer)
    {
        pSVData->mpSalTimer->Stop();
    }

    // Free active tasks
    ImplSchedulerData *pSchedulerData = pSVData->mpFirstSchedulerData;
    while ( pSchedulerData )
    {
        if ( pSchedulerData->mpScheduler )
            pSchedulerData->mpScheduler->mpSchedulerData = NULL;
        ImplSchedulerData* pNextSchedulerData = pSchedulerData->mpNext;
        delete pSchedulerData;
        pSchedulerData = pNextSchedulerData;
    }

    // Free "deleted" tasks
    pSchedulerData = pSVData->mpFreeSchedulerData;
    while ( pSchedulerData )
    {
        assert( !pSchedulerData->mpScheduler );
        ImplSchedulerData* pNextSchedulerData = pSchedulerData->mpNext;
        delete pSchedulerData;
        pSchedulerData = pNextSchedulerData;
    }

    pSVData->mpFirstSchedulerData = NULL;
    pSVData->mpFreeSchedulerData  = NULL;
    pSVData->mnTimerPeriod        = 0;

    delete pSVData->mpSalTimer;
    pSVData->mpSalTimer = 0;
}

void Scheduler::CallbackTaskScheduling( const bool bIdle )
{
    ImplSVData* pSVData = ImplGetSVData();
    pSVData->mbNeedsReschedule = true;
    Scheduler::ProcessTaskScheduling( bIdle );
}

inline void Scheduler::UpdateMinPeriod( ImplSchedulerData *pSchedulerData,
                                        const sal_uInt64 nTime, sal_uInt64 &nMinPeriod )
{
    if ( nMinPeriod > MIN_SLEEP_PERIOD )
        pSchedulerData->mpScheduler->UpdateMinPeriod( nTime, nMinPeriod );
    assert( nMinPeriod >= MIN_SLEEP_PERIOD );
}

void Scheduler::ProcessTaskScheduling( const bool bIdle )
{
    ImplSVData*        pSVData = ImplGetSVData();
    sal_uInt64         nTime = tools::Time::GetSystemTicks();
    if (!pSVData->mbNeedsReschedule &&
            (nTime < pSVData->mnLastUpdate + pSVData->mnTimerPeriod) )
        return;
    pSVData->mbNeedsReschedule = false;

    ImplSchedulerData* pSchedulerData = pSVData->mpFirstSchedulerData;
    ImplSchedulerData* pPrevSchedulerData = NULL;
    ImplSchedulerData *pPrevMostUrgent = NULL;
    ImplSchedulerData *pMostUrgent = NULL;
    sal_uInt64         nMinPeriod = MAX_SLEEP_PERIOD;

    while ( pSchedulerData )
    {
        if ( pSchedulerData->mbInScheduler )
            goto next_entry;

        // Should Task be released from scheduling?
        if ( !pSchedulerData->mpScheduler )
        {
            ImplSchedulerData* pNextSchedulerData = pSchedulerData->mpNext;
            if ( pPrevSchedulerData )
                pPrevSchedulerData->mpNext = pNextSchedulerData;
            else
                pSVData->mpFirstSchedulerData = pNextSchedulerData;
            pSchedulerData->mpNext = pSVData->mpFreeSchedulerData;
            pSVData->mpFreeSchedulerData = pSchedulerData;
            pSchedulerData = pNextSchedulerData;
            continue;
        }

        assert( pSchedulerData->mpScheduler );
        if ( !pSchedulerData->mpScheduler->ReadyForSchedule( nTime, bIdle ) )
            goto evaluate_entry;

        // if the priority of the current task is higher (numerical value is lower) than
        // the priority of the most urgent, the current task becomes the new most urgent
        if ( !pMostUrgent )
        {
            pPrevMostUrgent = pPrevSchedulerData;
            pMostUrgent = pSchedulerData;
        }
        else if ( pSchedulerData->mpScheduler->GetPriority() < pMostUrgent->mpScheduler->GetPriority() )
        {
            UpdateMinPeriod( pMostUrgent, nTime, nMinPeriod );
            pPrevMostUrgent = pPrevSchedulerData;
            pMostUrgent = pSchedulerData;
            goto next_entry;
        }

evaluate_entry:
        UpdateMinPeriod( pSchedulerData, nTime, nMinPeriod );

next_entry:
        pPrevSchedulerData = pSchedulerData;
        pSchedulerData = pSchedulerData->mpNext;
    }

    assert( !pSchedulerData );

    if ( pMostUrgent )
    {
        assert( pPrevMostUrgent != pMostUrgent );

        pMostUrgent->mnLastTime = nTime;
        UpdateMinPeriod( pMostUrgent, nTime, nMinPeriod );

        pMostUrgent->Invoke();

        // do some simple round-robin scheduling
        // nothing to do, if we're already the last element
        if ( pMostUrgent->mpScheduler && pMostUrgent != pPrevSchedulerData )
        {
            if ( pPrevMostUrgent )
                pPrevMostUrgent->mpNext = pMostUrgent->mpNext;
            else
                pSVData->mpFirstSchedulerData = pMostUrgent->mpNext;
            pPrevSchedulerData->mpNext = pMostUrgent;
            pMostUrgent->mpNext = NULL;
        }
    }

    // delete clock if no more timers available
    if ( nMinPeriod != MAX_SLEEP_PERIOD )
        Timer::ImplStartTimer( pSVData, nMinPeriod );
    else if ( pSVData->mpSalTimer )
        pSVData->mpSalTimer->Stop();

    pSVData->mnTimerPeriod = nMinPeriod;
    pSVData->mnLastUpdate = nTime;
}

void Scheduler::SetPriority( SchedulerPriority ePriority )
{
    mePriority = ePriority;
}

void Scheduler::Start()
{
    ImplSVData* pSVData = ImplGetSVData();
    if ( !mpSchedulerData )
    {
        // insert Scheduler
        if ( pSVData->mpFreeSchedulerData )
        {
            mpSchedulerData = pSVData->mpFreeSchedulerData;
            pSVData->mpFreeSchedulerData = mpSchedulerData->mpNext;
        }
        else
            mpSchedulerData = new ImplSchedulerData;
        mpSchedulerData->mpScheduler   = this;
        mpSchedulerData->mbInScheduler = false;
        mpSchedulerData->mpNext        = pSVData->mpFirstSchedulerData;
        pSVData->mpFirstSchedulerData  = mpSchedulerData;
    }

    assert( mpSchedulerData->mpScheduler == this );
    mpSchedulerData->mnLastTime  = tools::Time::GetSystemTicks();

    pSVData->mbNeedsReschedule = true;
}

void Scheduler::Stop()
{
    if ( !mpSchedulerData )
        return;
    Scheduler::SetDeletionFlags();
    assert( !mpSchedulerData );
}

Scheduler& Scheduler::operator=( const Scheduler& rScheduler )
{
    if ( IsActive() )
        Stop();

    mePriority = rScheduler.mePriority;

    if ( rScheduler.IsActive() )
        Start();

    return *this;
}

Scheduler::Scheduler(const sal_Char *pDebugName):
    mpSchedulerData(NULL),
    mpDebugName(pDebugName),
    mePriority(SchedulerPriority::HIGH)
{
}

Scheduler::Scheduler( const Scheduler& rScheduler ):
    mpSchedulerData(NULL),
    mpDebugName(rScheduler.mpDebugName),
    mePriority(rScheduler.mePriority)
{
    if ( rScheduler.IsActive() )
        Start();
}

Scheduler::~Scheduler()
{
    if ( mpSchedulerData )
        mpSchedulerData->mpScheduler = NULL;
}

SchedulerCallback::SchedulerCallback( const sal_Char *pDebugName )
    : Scheduler( pDebugName )
{
}

void SchedulerCallback::Invoke( SchedulerCallback *arg )
{
    maInvokeHandler.Call( arg );
}

void SchedulerCallback::Invoke()
{
    maInvokeHandler.Call( this );
}

void SchedulerAuto::SetDeletionFlags()
{
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
