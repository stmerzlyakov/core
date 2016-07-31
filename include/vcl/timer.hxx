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

#ifndef INCLUDED_VCL_TIMER_HXX
#define INCLUDED_VCL_TIMER_HXX

#include <tools/link.hxx>
#include <vcl/scheduler.hxx>

struct ImplSVData;

class VCL_DLLPUBLIC Timer : public SchedulerCallback
{
protected:
    sal_uInt64   mnTimeout;

    virtual bool ReadyForSchedule( const sal_uInt64 nTime, const bool bIdle ) SAL_OVERRIDE;
    virtual void UpdateMinPeriod( const sal_uInt64 nTime, sal_uInt64 &nMinPeriod ) SAL_OVERRIDE;

private:
    static void InitSystemTimer();

public:
    Timer( const sal_Char *pDebugName = NULL );

    void            SetTimeout( sal_uInt64 nTimeoutMs );
    sal_uInt64      GetTimeout() const { return mnTimeout; }
    virtual void    Start() SAL_OVERRIDE;

    void            SetTimeoutHdl( const Link<Timer*, void> &rLink );

    static void     ImplStartTimer( ImplSVData* pSVData, sal_uInt64 nMS );
};

inline void Timer::SetTimeoutHdl( const Link<Timer*, void> &rLink )
{
    SetInvokeHandler( Link<SchedulerCallback*, void>(
        static_cast<SchedulerCallback*>( rLink.GetInstance() ),
        reinterpret_cast< Link<SchedulerCallback*, void>::Stub* >( rLink.GetFunction()) ));
}

/// An auto-timer is a multi-shot timer re-emitting itself at
/// interval until destroyed.
class VCL_DLLPUBLIC AutoTimer : public Timer, public SchedulerAuto
{
public:
    AutoTimer( const sal_Char *pDebugName = NULL );
};

#endif // INCLUDED_VCL_TIMER_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
