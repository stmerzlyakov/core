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

#include <tools/time.hxx>
#include <vcl/timer.hxx>
#include <saltimer.hxx>
#include <svdata.hxx>
#include <salinst.hxx>

void Timer::ImplStartTimer( ImplSVData* pSVData, sal_uInt64 nMS )
{
    InitSystemTimer();

    if ( nMS < MIN_SLEEP_PERIOD )
        nMS = MIN_SLEEP_PERIOD;

    // Assume underlying timers are recurring timers, if same period - just wait.
    if ( nMS != pSVData->mnTimerPeriod )
    {
        pSVData->mnTimerPeriod = nMS;
        pSVData->mpSalTimer->Start( nMS );
    }
}

bool Timer::ReadyForSchedule( const sal_uInt64 nTime, bool /* bIdle */ )
{
    return (mpSchedulerData->mnLastTime + mnTimeout) <= nTime;
}

void Timer::UpdateMinPeriod( const sal_uInt64 nTime, sal_uInt64 &nMinPeriod )
{
    sal_uInt64 nWakeupTime = mpSchedulerData->mnLastTime + mnTimeout;
    if( nWakeupTime <= nTime )
        nMinPeriod = MIN_SLEEP_PERIOD;
    else
    {
        sal_uInt64 nSleepTime = nWakeupTime - nTime;
        if( nSleepTime < nMinPeriod )
            nMinPeriod = nSleepTime;
    }
}

/**
 * Initialize the platform specific timer on which all the
 * platform independent timers are built
 */
void Timer::InitSystemTimer()
{
    ImplSVData* pSVData = ImplGetSVData();
    if( ! pSVData->mpSalTimer )
    {
        pSVData->mnTimerPeriod = MAX_SLEEP_PERIOD;
        pSVData->mpSalTimer = pSVData->mpDefInst->CreateSalTimer();
        pSVData->mpSalTimer->SetCallback( CallbackTaskScheduling );
    }
}

Timer::Timer( const sal_Char *pDebugName ) : SchedulerCallback( pDebugName )
{
    mnTimeout    = MIN_SLEEP_PERIOD;
    mePriority   = SchedulerPriority::DEFAULT;
}

void Timer::Start()
{
    Scheduler::Start();

    ImplSVData* pSVData = ImplGetSVData();
    if ( mnTimeout < pSVData->mnTimerPeriod )
        Timer::ImplStartTimer( pSVData, mnTimeout );
}

void Timer::SetTimeout( sal_uInt64 nNewTimeout )
{
    mnTimeout = nNewTimeout;
    // if timer is active then renew clock
    if ( IsActive() )
    {
        ImplSVData* pSVData = ImplGetSVData();
        if ( mnTimeout < pSVData->mnTimerPeriod )
            Timer::ImplStartTimer( pSVData, mnTimeout );
    }
}

AutoTimer::AutoTimer( const sal_Char *pDebugName )
    : Timer( pDebugName ), SchedulerAuto( pDebugName )
{
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
