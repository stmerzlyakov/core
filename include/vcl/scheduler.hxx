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

#ifndef INCLUDED_VCL_SCHEDULER_HXX
#define INCLUDED_VCL_SCHEDULER_HXX

#include <vcl/dllapi.h>

#define MIN_SLEEP_PERIOD   1
#define MAX_SLEEP_PERIOD   SAL_MAX_UINT64

class Scheduler;
struct ImplSchedulerData
{
    ImplSchedulerData*  mpNext;         ///< Pointer to the next element in list
    Scheduler*          mpScheduler;    ///< Pointer to VCL Scheduler instance, NULL if to be removed
    bool                mbInScheduler;  ///< Is the Scheduler / task currently processed? Used to prevent recursive processing.
    sal_uInt64          mnLastTime;     ///< Timestamp, when the task was scheduled / invoked / running last

    void Invoke();
};

/// Higher priority = smaller number!
enum class SchedulerPriority {
    HIGHEST,       ///< These events should run very fast!
    DEFAULT,       ///< Default priority used, e.g. the default timer priority
    HIGH_IDLE,     ///< Important idle events to be run before processing drawing events
    RESIZE,        ///< Resize runs before repaint, so we won't paint twice
    REPAINT,       ///< All repaint events should go in here
    DEFAULT_IDLE,  ///< Default idle priority
    LOWEST
};

class VCL_DLLPUBLIC Scheduler
{
    friend struct ImplSchedulerData;

private:
    static inline void UpdateMinPeriod( ImplSchedulerData *pSchedulerData,
                                        const sal_uInt64 nTime, sal_uInt64 &nMinPeriod );

protected:
    ImplSchedulerData  *mpSchedulerData;  ///< Pointer to the element in scheduler list, when active
    const sal_Char     *mpDebugName;      ///< Name to identify the Scheduler origin (for debugging)
    SchedulerPriority   mePriority;       ///< Scheduler priority

    virtual void SetDeletionFlags();
    virtual bool ReadyForSchedule( const sal_uInt64 nTime, const bool bIdle ) = 0;
    // determine smallest time period to sleep
    virtual void UpdateMinPeriod( const sal_uInt64 nTime, sal_uInt64 &nMinPeriod ) = 0;

protected:
    /**
     * Process the pending task with the highest priority
     *
     * This function is called by the yield timer. It actually just calls
     * Scheduler::ProcessTaskScheduling and enforces recheduling by setting
     * pSVData->mbNeedsReschedule.
     *
     * @param bIdle if false, don't process tasks with priority >= DEFAULT_IDLE
     */
    static void CallbackTaskScheduling( const bool bIdle );

public:
    Scheduler( const sal_Char *pDebugName = NULL );
    Scheduler( const Scheduler& rScheduler );
    virtual ~Scheduler();
    Scheduler& operator=( const Scheduler& rScheduler );

    void              SetPriority( SchedulerPriority ePriority );
    SchedulerPriority GetPriority() const { return mePriority; }

    void            SetDebugName( const sal_Char *pDebugName ) { mpDebugName = pDebugName; }
    const sal_Char* GetDebugName() const { return mpDebugName; }

    virtual void    Invoke() = 0;

    virtual void    Start();
    virtual void    Stop();

    inline bool     IsActive() const;

    static void     ImplDeInitScheduler();

    /**
     * Process the pending task with highest priority
     *
     * @param bIdle if false, don't process tasks with priority >= DEFAULT_IDLE
     */
    static void     ProcessTaskScheduling( const bool bIdle );
};

inline bool Scheduler::IsActive() const
{
    return NULL != mpSchedulerData;
}

class VCL_DLLPUBLIC SchedulerCallback : public virtual Scheduler
{
public:
    typedef Link<SchedulerCallback*, void> InvokeHandler;

private:
    InvokeHandler   maInvokeHandler;  ///< Default handler to be invoked when scheduled

protected:
    SchedulerCallback( const sal_Char *pDebugName = NULL );

public:
    // Call the invoke handler with *this* as parameter
    virtual void    Invoke() SAL_OVERRIDE;
    void            Invoke( SchedulerCallback* arg );
    void            SetInvokeHandler( const InvokeHandler &rLink ) { maInvokeHandler = rLink; }
    bool            HasInvokeHandler() const { return maInvokeHandler.IsSet(); }
};

class VCL_DLLPUBLIC SchedulerAuto : public virtual Scheduler
{
protected:
    SchedulerAuto( const sal_Char *pDebugName = NULL ): Scheduler( pDebugName ) {}
    virtual void    SetDeletionFlags() SAL_OVERRIDE;
};

#endif // INCLUDED_VCL_SCHEDULER_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
