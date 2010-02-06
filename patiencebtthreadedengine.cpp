#include "patiencebtthreadedengine.h"
#include <stdio.h>

PatienceBTThreadedEngine::PatienceBTThreadedEngine(MPatineceBTEngineCaller& aCaller, const uint8_t* aSvc_uuid_int):PatienceBTClientEngine( aCaller, aSvc_uuid_int)
{ 
   QObject::connect(this, SIGNAL(EngineStateChangeSignal(int)),this, SLOT (EngineStateChangeSlot(int)));
    iThread = NULL;
    iLiveSocketToDisconnect = 0;
}
PatienceBTThreadedEngine::~PatienceBTThreadedEngine()
{
    if(iLiveSocketToDisconnect!=0)
        Disconnect(); //this would stop running rfcomm thread, if not - in some cases user needs to close the mobile phone's bt to unblock this.

    if(iThread && iThread->isRunning())
        iThread->wait();

    delete iThread;
}

void PatienceBTThreadedEngine::Disconnect()
{
    DoDisconnect();
}

///////////search

void PatienceBTThreadedEngine::CSearchThread::run()
{
    iFather.DoSearch();
}

void PatienceBTThreadedEngine::CSDPThread::run()
{
    iFather.DoSDP();    
}


//adapted from http://people.csail.mit.edu/albert/bluez-intro/x502.html
void PatienceBTThreadedEngine::CRFCOMMThread::run()
{
    iFather.DoRFCOMM();
}

bool PatienceBTThreadedEngine::StartSearch()
{
    if(iThread && iThread->isRunning())
    {
        return false;
    }

    //if comes here measn thread has finished
    delete iThread;

    iThread = new CSearchThread(*this);
    iThread->start();

    return true;
}


void PatienceBTThreadedEngine::GetDevListClone(QList<TBtDevInfo>& aDevList)
{
    iMutex.lock();
    aDevList = iDevList;
    iMutex.unlock();
}

void PatienceBTThreadedEngine::StartPrevdev(QByteArray& ba)
{

    emit EngineStateChangeSignal(PatienceBTThreadedEngine::EBtSearching); //let the ui prepare to go to sdp state
    iDevList.clear();

    TBtDevInfo devinfo;
    CopyBDADDR((uint8_t*)ba.data(),(uint8_t*)devinfo.iAddr);

    iDevList.append(devinfo);
    iSelectedIndex = 0;
    StartSDPToSelectedDev(0);
}

void PatienceBTThreadedEngine::StartSDPToSelectedDev(int aSelIndex)
{
                    iSelectedIndex = aSelIndex;
                    qDebug("user selected index: %d",aSelIndex);
                    QString str;
                    str = iDevList[aSelIndex].iName;
                    str += " selected, preparing to search for service...";
                    emit EngineStatusMessageSignal(str);


                    //////////////// start sdp search thread
                    if(iThread && iThread->isRunning())
                     {
                        perror("Warning: waiting on scan thread? actually no thread should be active now...");
                        iThread->wait();
                        perror("wait thread ended");
                     }

                    //if comes here measn thread has finished
                    perror("deleting iThread");

                    delete iThread;
                    iThread = NULL;

                    iThread = new CSDPThread(*this);
                    perror("created sdp thread");
                    iThread->start();
                    perror("started sdp thread");
                    emit EngineStateChangeSignal(EBtSearchingSDP);
                    ////////////////

}
void PatienceBTThreadedEngine::EngineStateChangeSlot(int aState)
{
    switch(aState)
    {
    case PatienceBTThreadedEngine::EBtIdle:
        iLiveSocketToDisconnect = 0;
        break;
    case PatienceBTThreadedEngine::EBtSearching:
        break;
    case PatienceBTThreadedEngine::EBtSelectingPhoneToSDP:
        {
            if(iDevList.isEmpty())
            {
                //QMessageBox::information(iParentWindow, tr("No nearby Bluetooth devices found"),tr("No nearby Bluetooth devices found.\r\n\r\nPlease install or start (if already installed) the mobile program on your phone and try again."));

                emit EngineStatusMessageSignal("No nearby Bluetooth devices found");
                emit EngineErrorSignal(EInquiryFoundNoDevices);
                emit EngineStateChangeSignal(EBtIdle);
            }
            else
            {
                int aSelIndex=-1;
                iCaller.OnSelectBtDevice(iDevList,aSelIndex);

                if( aSelIndex >=0 ) //selected
                {
                 qDebug("aselindex %d",aSelIndex);
                 StartSDPToSelectedDev(aSelIndex);
                }
                else //closed/cancelled
                {
                    emit EngineStateChangeSignal(EBtIdle);
                    emit EngineStatusMessageSignal("Connect Cancelled");
                }
            }
        }
        break;
    case PatienceBTThreadedEngine::EBtSearchingSDP:
    {




    }
        break;

    case PatienceBTThreadedEngine::EBtSearchingSDPDone:
    {
            if(iRFCOMMChannel<0) //not found
            {
                emit EngineStatusMessageSignal("mobile program not started");
                //QMessageBox::information(iParentWindow, tr("program not started on phone"),tr("Can't find the mobile-side program running on selected mobile.\r\n\r\nPlease install/start the mobile program on your phone and try again."));
                emit EngineErrorSignal(EServiceNotFoundOnDevice);
                emit EngineStateChangeSignal(EBtIdle);
            }
            else
            {
                emit EngineStatusMessageSignal("channel found");

                //start connect RFCOMM thread
                //////////////
                if(iThread && iThread->isRunning())
                 {
                iThread->wait();
                 }

                //if comes here measn thread has finished
                delete iThread;

                iThread = new CRFCOMMThread(*this);
                iThread->start();
                ////////////////

            }
    }
    break;



    case PatienceBTThreadedEngine::EBtConnectingRFCOMM:

        break;
    case PatienceBTThreadedEngine::EBtConnectionActive:

        break;
    case PatienceBTThreadedEngine::EBtDisconnected:
        iLiveSocketToDisconnect = 0;
        break;
    default:

        break;

    }
}
