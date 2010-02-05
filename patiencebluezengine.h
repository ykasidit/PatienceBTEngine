/*
    Copyright (C) 2009 Kasidit Yusuf.

    This file is part of PatienceBTEngine.

    PatienceBTEngine is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PatienceBTEngine is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PatienceBTEngine.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef PatienceBLUEZENGINE_H
#define PatienceBLUEZENGINE_H

#include "patiencebtengine.h"

#include <QList>
#include <QString>
#include <QThread>
#include <QMutex>

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

class PatienceBlueZEngine : public PatienceBTClientEngine
{
    Q_OBJECT

    public:
    PatienceBlueZEngine(MPatineceBTEngineCaller& aCaller, const uint8_t* aSvc_uuid_int);
    virtual ~PatienceBlueZEngine();

    signals:
    virtual void EngineStateChangeSignal(int aState);
    virtual void EngineStatusMessageSignal(QString str);    
    virtual void RFCOMMDataReceivedSignal(QByteArray ba);
    virtual void EngineErrorSignal(int aError);

    public slots:
    void EngineStateChangeSlot(int aState); // to detect/handle our own state change made by bt operation threads

    public:

    ///////////bt operation thread classes and functions

        class CBtEngineThread : public QThread
        {
            public:
            CBtEngineThread(PatienceBlueZEngine &aFather):iFather(aFather){}
            PatienceBlueZEngine &iFather;
        };

        class CSearchThread : public CBtEngineThread
        {
         public:
             CSearchThread(PatienceBlueZEngine &aFather):CBtEngineThread(aFather){}
             void run();
        };

        class CSDPThread : public CBtEngineThread
        {
         public:
             CSDPThread(PatienceBlueZEngine &aFather):CBtEngineThread(aFather){}
             void run();
        };

        class CRFCOMMThread : public CBtEngineThread
        {
         public:
             CRFCOMMThread(PatienceBlueZEngine &aFather):CBtEngineThread(aFather){}
             void run();

             struct sockaddr_rc addr;
             int s, status;
        };


        friend class CSearchThread;
        friend class CSDPThread;

        virtual void StartPrevdev(QByteArray& ba);
        virtual bool StartSearch();
        virtual void StartSDPToSelectedDev(int aSelIndex);
        virtual void GetDevListClone(QList<TBtDevInfo>& aDevList);
        virtual void Disconnect();
    /////////////////

private:

    ////////////////////////for shared stuff between current and result thread like iDevlist
    QThread* iThread;
    QMutex iMutex;
    QList<TBtDevInfo> iDevList;
    int iRFCOMMChannel;
    int iLiveSocketToDisconnect;
    //////////////////////////////

    int iSelectedIndex;    
};

#endif // PatienceBLUEZENGINE_H
