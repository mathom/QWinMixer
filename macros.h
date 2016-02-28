#ifndef MACROS_H
#define MACROS_H

#define EXIT_ON_ERROR(hres, msg)  \
              if (FAILED(hres)) { throw msg; }
#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }
#define TASK_FREE(ptr) \
              if ((ptr) != NULL) \
                { CoTaskMemFree(ptr); (ptr) = NULL; }

inline QString lastErrorMessage() {
    LPWSTR lpMsgBuf = NULL;

    DWORD dw = GetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        lpMsgBuf,
        0, NULL );

    QString msg = QString::fromWCharArray(lpMsgBuf);

    LocalFree(lpMsgBuf);

    return msg;
}

#endif // MACROS_H
