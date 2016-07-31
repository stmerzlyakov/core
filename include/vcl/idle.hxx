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

#ifndef INCLUDED_VCL_IDLE_HXX
#define INCLUDED_VCL_IDLE_HXX

#include <tools/link.hxx>
#include <vcl/scheduler.hxx>

class VCL_DLLPUBLIC Idle : public SchedulerCallback
{
protected:
    virtual bool ReadyForSchedule( const sal_uInt64 nTime, const bool bIdle ) SAL_OVERRIDE;
    virtual void UpdateMinPeriod( const sal_uInt64 nTime, sal_uInt64 &nMinPeriod ) SAL_OVERRIDE;

public:
    Idle( const sal_Char *pDebugName = NULL );

    void SetIdleHdl( const Link<Idle*, void> &rLink );

    virtual void Start() SAL_OVERRIDE;
};

inline void Idle::SetIdleHdl( const Link<Idle*, void> &rLink )
{
    SetInvokeHandler( Link<SchedulerCallback*, void>(
        static_cast<SchedulerCallback*>( rLink.GetInstance() ),
        reinterpret_cast< Link<SchedulerCallback*, void>::Stub* >( rLink.GetFunction()) ));
}

#endif // INCLUDED_VCL_IDLE_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
