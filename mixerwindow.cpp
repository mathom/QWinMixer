#include "mixerwindow.h"
#include "ui_mixerwindow.h"

#include <QMessageBox>
#include <QDebug>
#include <QDir>

#include <Mmdeviceapi.h>
#include <Audiopolicy.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <Psapi.h>

#include "macros.h"

MixerWindow::MixerWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MixerWindow),
    _sessionManager(NULL),
    _cRef(1)
{
    ui->setupUi(this);

    //HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        error(tr("Could not initialize COM: %1").arg(lastErrorMessage()));
    }

    loadSliders();
}

MixerWindow::~MixerWindow()
{
    delete ui;

    HRESULT hr = sessionManager()->UnregisterSessionNotification(this);
    if (FAILED(hr)) {
        error(QString("Could not unregister session notification: %1").arg(lastErrorMessage()));
    }

    clearSliders();

    SAFE_RELEASE(_sessionManager);
}

ULONG MixerWindow::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG MixerWindow::Release()
{
    ULONG ulRef = InterlockedDecrement(&_cRef);
    if (0 == ulRef)
    {
        //delete this;
    }
    return ulRef;
}

HRESULT MixerWindow::QueryInterface(const IID &riid, void **ppv)
{
    if (IID_IUnknown == riid)
    {
        AddRef();
        *ppv = (IUnknown*)this;
    }
    else if (__uuidof(IAudioSessionNotification) == riid)
    {
        AddRef();
        *ppv = (IAudioSessionNotification*)this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    return S_OK;
}

HRESULT MixerWindow::OnSessionCreated(IAudioSessionControl *pNewSession)
{
    qDebug() << "detected new session";
    if (pNewSession)
    {
        addSlider(pNewSession);
    }
    return S_OK;
}

void MixerWindow::destroySlider()
{
    Slider *slider = qobject_cast<Slider*>(sender());
    if (slider) {
        sliders.removeOne(slider);
        ui->centralWidget->layout()->removeWidget(slider);
        delete slider;
    }
}

IAudioSessionManager2 *MixerWindow::sessionManager()
{
    HRESULT hr = S_OK;
    IMMDeviceEnumerator *pEnumerator = NULL;
    IMMDevice *pEndpoint = NULL;

    if (_sessionManager == NULL) {
        try {
            hr = CoCreateInstance(
                   __uuidof(MMDeviceEnumerator), NULL,
                   CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                   (void**)&pEnumerator);
            EXIT_ON_ERROR(hr, tr("Could not create device enumerator"));

            hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia,
                                                      &pEndpoint);
            EXIT_ON_ERROR(hr, tr("Could not get default audio endpoint"));

            hr = pEndpoint->Activate(
                __uuidof(IAudioSessionManager2), CLSCTX_ALL,
                NULL, (void**)&_sessionManager);
            EXIT_ON_ERROR(hr, tr("Could not activate audio session manager"));
            //_sessionManager->AddRef();

            hr = _sessionManager->RegisterSessionNotification(this);
            EXIT_ON_ERROR(hr, tr("Could not register session notification: %1").arg(lastErrorMessage()));

        } catch (const QString &msg) {
            error(QString("%1: %2").arg(msg).arg(lastErrorMessage()));
        }
    }

    SAFE_RELEASE(pEndpoint);
    SAFE_RELEASE(pEnumerator);

    return _sessionManager;
}

void MixerWindow::clearSliders()
{
    foreach(Slider *slider, sliders) {
        delete slider;
    }
    sliders.clear();
}

void MixerWindow::addSlider(IAudioSessionControl *sessionControl)
{
    HRESULT hr = S_OK;

    IAudioSessionControl2 *pSessionControl2 = NULL;
    ISimpleAudioVolume *pVolume = NULL;

    GUID groupID;

    LPWSTR pswSession = NULL;
    WCHAR buff[2048];

    HANDLE h = NULL;

    QString name, identifier, icon, executable;

    try {
        hr = sessionControl->QueryInterface(
                __uuidof(IAudioSessionControl2), (void**) &pSessionControl2);
        EXIT_ON_ERROR(hr, tr("Could not get extended session information"));

        if (pSessionControl2->IsSystemSoundsSession()) {
            hr = sessionControl->GetDisplayName(&pswSession);
            EXIT_ON_ERROR(hr, tr("Could not get session display name"));
            name = QString::fromWCharArray(pswSession);
            TASK_FREE(pswSession);

            hr = pSessionControl2->GetSessionIdentifier(&pswSession);
            EXIT_ON_ERROR(hr, tr("Could not get session identifier"));
            identifier = QString::fromWCharArray(pswSession);
            TASK_FREE(pswSession);

            hr = pSessionControl2->GetGroupingParam(&groupID);
            EXIT_ON_ERROR(hr, tr("Could not get session identifier"));
            identifier = QString::fromWCharArray(pswSession);
            TASK_FREE(pswSession);

            hr = pSessionControl2->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pVolume);
            EXIT_ON_ERROR(hr, tr("Could not get volume control"));

            DWORD pid;
            hr = pSessionControl2->GetProcessId(&pid);
            EXIT_ON_ERROR(hr, tr("Could not get session process ID"));

            h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);


            if (h != NULL) {
                DWORD length = sizeof(buff)-1;
                if (!QueryFullProcessImageName (h, 0, buff, &length)) {
                    throw tr("Could not get module file name from process ID %1").arg(pid);
                }
                executable = QString::fromWCharArray(buff);

                if (icon.isEmpty()) {
                    icon = executable;
                }

                if (name.isEmpty()) {
                    name = executable.split(QDir::separator()).last();
                }

                if (name.endsWith(".exe")) {
                    name.chop(4);
                }
            }
            else {
                //qDebug() << "error opening process" << h << pid << GetLastError() << lastErrorMessage();
            }

            qDebug() << "adding slider" << name;

            Slider *slider = new Slider(this, name, icon, pVolume, sessionControl);
            sliders.append(slider);

            QBoxLayout *layout = qobject_cast<QBoxLayout*>(ui->centralWidget->layout());
            bool inserted = false;
            for (int i=0; i<layout->count(); i++) {
                QWidget *widget = layout->itemAt(i)->widget();
                Slider *oldSlider = qobject_cast<Slider*>(widget);
                if (oldSlider) {
                    if (oldSlider->name() > slider->name()) {
                        qDebug() << "inserting" << slider->name() << "at" << qMax(i-1, 0);
                        layout->insertWidget(qMax(i-1, 0), slider);
                        inserted = true;
                        break;
                    }
                }
            }

            if (!inserted) {
                layout->addWidget(slider);
            }
        }
    } catch (const QString &msg) {
        error(QString("%1: %2").arg(msg).arg(lastErrorMessage()));
        SAFE_RELEASE(pVolume);
        SAFE_RELEASE(sessionControl);
    }

    if (h) {
        CloseHandle(h);
    }
    TASK_FREE(pswSession);
    SAFE_RELEASE(pSessionControl2);
}

void MixerWindow::error(const QString &message)
{
    QMessageBox::critical(parentWidget(), tr("Error!"), message);
}

void MixerWindow::loadSliders()
{
    HRESULT hr = S_OK;

    IAudioSessionEnumerator *pSessionList = NULL;
    IAudioSessionControl *pSessionControl = NULL;

    int cbSessionCount = 0;

    try {
        hr = sessionManager()->GetSessionEnumerator(&pSessionList);
        EXIT_ON_ERROR(hr, tr("Could not list audio sessions"));

        hr = pSessionList->GetCount(&cbSessionCount);
        EXIT_ON_ERROR(hr, tr("Could not get audio session count"));

        for (int index = 0; index < cbSessionCount; index++)
        {
            hr = pSessionList->GetSession(index, &pSessionControl);
            EXIT_ON_ERROR(hr, tr("Could not get session information"));

            addSlider(pSessionControl);
        }
    } catch (const QString &msg) {
        error(QString("%1: %2").arg(msg).arg(lastErrorMessage()));
        SAFE_RELEASE(pSessionControl);
    }

    SAFE_RELEASE(pSessionList);
}
