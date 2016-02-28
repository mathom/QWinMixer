#ifndef MIXERWINDOW_H
#define MIXERWINDOW_H

#include <QMainWindow>

#include <Audiopolicy.h>

#include "slider.h"

struct IAudioSessionManager2;
struct IAudioSessionControl;

namespace Ui {
class MixerWindow;
}

class MixerWindow : public QMainWindow, public IAudioSessionNotification
{
    Q_OBJECT

public:
    explicit MixerWindow(QWidget *parent = 0);
    ~MixerWindow();

    // Windows garbage
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv);

    HRESULT OnSessionCreated(IAudioSessionControl *pNewSession);

public slots:
    void destroySlider();

private:
    IAudioSessionManager2 *sessionManager();
    void loadSliders();
    void clearSliders();

    void addSlider(IAudioSessionControl *sessionControl);

    void error(const QString &message);

    Ui::MixerWindow *ui;

    QList<Slider*> sliders;

    IAudioSessionManager2 *_sessionManager;

    LONG _cRef;
};

#endif // MIXERWINDOW_H
