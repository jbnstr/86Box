/*
 * 86Box        A hypervisor and IBM PC system emulator that specializes in
 *              running old operating systems and software designed for IBM
 *              PC systems and compatibles from 1981 through fairly recent
 *              system designs based on the PCI bus.
 *
 *              This file is part of the 86Box distribution.
 *
 *              Definitions for platform specific serial to host passthrough
 *
 *
 * Authors:     Andreas J. Reichel <webmaster@6th-dimension.com>,
 *              Jasmine Iwanek <jasmine@iwanek.co.uk>
 *
 *              Copyright 2021      Andreas J. Reichel
 *              Copyright 2021-2023 Jasmine Iwanek
 */

#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <wchar.h>

#include <86box/86box.h>
#include <86box/log.h>
#include <86box/timer.h>
#include <86box/plat.h>
#include <86box/device.h>
#include <86box/serial_passthrough.h>
#include <86box/plat_serial_passthrough.h>
#include <86box/ui.h>

#include "win/win_error_message.h"
#include <windows.h>

#define LOG_PREFIX "serial_passthrough: "

void
plat_serpt_close(void *priv)
{
    serial_passthrough_t *dev = (serial_passthrough_t *) priv;

#if 0
    fclose(dev->master_fd);
#endif
    FlushFileBuffers((HANDLE) dev->master_fd);
    if (dev->mode == SERPT_MODE_VCONSRV)
        DisconnectNamedPipe((HANDLE) dev->master_fd);
    if (dev->mode == SERPT_MODE_HOSTSER) {
        SetCommState((HANDLE) dev->master_fd, (DCB *) dev->backend_priv);
        free(dev->backend_priv);
    }
    CloseHandle((HANDLE) dev->master_fd);
}

static void
plat_serpt_write_vcon_srvr(serial_passthrough_t *dev, uint8_t data)
{
#if 0
     fd_set wrfds;
     int res;
#endif

    /* We cannot use select here, this would block the hypervisor! */
#if 0
     FD_ZERO(&wrfds);
     FD_SET(ctx->master_fd, &wrfds);

     res = select(ctx->master_fd + 1, NULL, &wrfds, NULL, NULL);

     if (res <= 0)
         return;
#endif

    /* just write it out */
#if 0
     fwrite(dev->master_fd, &data, 1);
#endif
    DWORD bytesWritten = 0;
    WriteFile((HANDLE) dev->master_fd, &data, 1, &bytesWritten, NULL);
}

void
plat_serpt_write_vcon_clnt(serial_passthrough_t *dev, uint8_t data)
{
    // The function will always wait for completion (due to INFINITE timeout),
    // making it effectively synchronous. This is the intended behavior, since
    // the named pipe (dev->master_fd) is created with FILE_FLAG_OVERLAPPED.
    // This is necessary because plat_serpt_read_vcon_clnt needs to be 
    // non-blocking.

    // Reset the event and overlapped structure.
    ResetEvent(dev->ov_write_event);
    memset(&dev->ov_write, 0, sizeof(dev->ov_write));
    dev->ov_write.hEvent = dev->ov_write_event;

    // Attempt async write.
    if (!WriteFile((HANDLE) dev->master_fd, &data, 1, NULL, &dev->ov_write)) {
        DWORD err = GetLastError();
        if (err != ERROR_IO_PENDING) {
            HANDLE_WINAPI_ERROR_2(WriteFile, err);
            return;
        }

        // Wait for completion.
        if (WaitForSingleObject(dev->ov_write_event, INFINITE) != WAIT_OBJECT_0) {
            HANDLE_WINAPI_ERROR_2(WaitForSingleObject, GetLastError());
            return;
        }

        // Verify the operation completed successfully.
        DWORD bytesWritten;
        if (!GetOverlappedResult((HANDLE) dev->master_fd, &dev->ov_write, &bytesWritten, FALSE)) {
            HANDLE_WINAPI_ERROR_2(GetOverlappedResult, GetLastError());
            return;
        }
    }
}

void
plat_serpt_set_params(void *priv)
{
    const serial_passthrough_t *dev = (serial_passthrough_t *) priv;

    if (dev->mode == SERPT_MODE_HOSTSER) {
        DCB serialattr = { 0 };
        GetCommState((HANDLE) dev->master_fd, &serialattr);
#define BAUDRATE_RANGE(baud_rate, min, max)    \
    if (baud_rate >= min && baud_rate < max) { \
        serialattr.BaudRate = min;             \
    }

        BAUDRATE_RANGE(dev->baudrate, 110, 300);
        BAUDRATE_RANGE(dev->baudrate, 300, 600);
        BAUDRATE_RANGE(dev->baudrate, 600, 1200);
        BAUDRATE_RANGE(dev->baudrate, 1200, 2400);
        BAUDRATE_RANGE(dev->baudrate, 2400, 4800);
        BAUDRATE_RANGE(dev->baudrate, 4800, 9600);
        BAUDRATE_RANGE(dev->baudrate, 9600, 14400);
        BAUDRATE_RANGE(dev->baudrate, 14400, 19200);
        BAUDRATE_RANGE(dev->baudrate, 19200, 38400);
        BAUDRATE_RANGE(dev->baudrate, 38400, 57600);
        BAUDRATE_RANGE(dev->baudrate, 57600, 115200);
        BAUDRATE_RANGE(dev->baudrate, 115200, 0xFFFFFFFF);

        serialattr.ByteSize = dev->data_bits;
        serialattr.StopBits = (dev->serial->lcr & 0x04) ? TWOSTOPBITS : ONESTOPBIT;
        if (!(dev->serial->lcr & 0x08)) {
            serialattr.fParity = 0;
            serialattr.Parity  = NOPARITY;
        } else {
            serialattr.fParity = 1;
            if (dev->serial->lcr & 0x20) {
                serialattr.Parity = MARKPARITY + !!(dev->serial->lcr & 0x10);
            } else {
                serialattr.Parity = ODDPARITY + !!(dev->serial->lcr & 0x10);
            }
        }

        SetCommState((HANDLE) dev->master_fd, &serialattr);
#undef BAUDRATE_RANGE
    }
}

void
plat_serpt_write(void *priv, uint8_t data)
{
    serial_passthrough_t *dev = (serial_passthrough_t *) priv;

    switch (dev->mode) {
        case SERPT_MODE_VCONCLNT:
            plat_serpt_write_vcon_clnt(dev, data);
            break;
        case SERPT_MODE_VCONSRV:
        case SERPT_MODE_HOSTSER:
            plat_serpt_write_vcon_srvr(dev, data);
            break;
        default:
            break;
    }
}

uint8_t
plat_serpt_read_vcon_srvr(serial_passthrough_t *dev, uint8_t *data)
{
    DWORD bytesRead = 0;
    ReadFile((HANDLE) dev->master_fd, data, 1, &bytesRead, NULL);
    return !!bytesRead;
}

uint8_t
plat_serpt_read_vcon_clnt(serial_passthrough_t *dev, uint8_t *data)
{
    DWORD bytesRead = 0;

    // If no read is pending, issue one.
    if (!dev->ov_read_pending) {
        ResetEvent(dev->ov_read_event);
        if (!ReadFile((HANDLE) dev->master_fd, dev->ov_read_buffer, 1, NULL, &dev->ov_read)) {
            DWORD err = GetLastError();
            if (err == ERROR_IO_PENDING) {
                dev->ov_read_pending = TRUE;
            } else {
                HANDLE_WINAPI_ERROR_2(ReadFile, err);
            }
        }
    }

    // Poll for completion.
    if (GetOverlappedResult((HANDLE) dev->master_fd, &dev->ov_read, &bytesRead, FALSE)) {
        *data                = dev->ov_read_buffer[0];
        dev->ov_read_pending = FALSE;
        return 1;
    }

    return 0; // Not ready yet.
}

int
plat_serpt_read(void *priv, uint8_t *data)
{
    serial_passthrough_t *dev = (serial_passthrough_t *) priv;
    int                   res = 0;

    switch (dev->mode) {
        case SERPT_MODE_VCONCLNT:
            res = plat_serpt_read_vcon_clnt(dev, data);
            break;
        case SERPT_MODE_VCONSRV:
        case SERPT_MODE_HOSTSER:
            res = plat_serpt_read_vcon_srvr(dev, data);
            break;
        default:
            break;
    }
    return res;
}

static int
setup_pipe_server(serial_passthrough_t *dev, char const *const ascii_pipe_name)
{
    if (dev == NULL)
        return 0;

    dev->master_fd = (intptr_t) CreateNamedPipeA(ascii_pipe_name,
                                                 PIPE_ACCESS_DUPLEX,
                                                 PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_NOWAIT,
                                                 1,     // Max 1 instance.
                                                 65536, // Number of bytes reserved for the output buffer.
                                                 65536, // Number of bytes reserved for the input buffer.
                                                 NMPWAIT_USE_DEFAULT_WAIT,
                                                 NULL); // Default security descriptor.

    if (dev->master_fd == (intptr_t) INVALID_HANDLE_VALUE) {
        wchar_t errorMsg[512]  = { 0 };
        wchar_t finalMsg[1024] = { 0 };
        DWORD   error          = GetLastError();
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), errorMsg, 1024, NULL);
        swprintf(finalMsg, 1024, L"Named Pipe (server, named_pipe=\"%hs\", port=COM%d): %ls\n", ascii_pipe_name, dev->port + 1, errorMsg);
        ui_msgbox(MBX_ERROR | MBX_FATAL, finalMsg);
        return 0;
    }

    return 1;
}

static int
connect_to_pipe_server(serial_passthrough_t *dev, char const *const ascii_pipe_name)
{
    if (dev == NULL)
        return 0;

    int  result      = 0;
    char szMsg[1024] = { 0 };

    snprintf(szMsg, sizeof(szMsg) / sizeof(szMsg[0]),
             "Server not available (named_pipe=\"%hs\", port=COM%d). Try again? ([No] ends the application.)",
             ascii_pipe_name, dev->port + 1);

    do {
        dev->master_fd = (intptr_t) CreateFileA(ascii_pipe_name,
                                                GENERIC_READ | GENERIC_WRITE,
                                                0,
                                                NULL,
                                                OPEN_EXISTING,
                                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                                NULL);

        if (dev->master_fd != (intptr_t) INVALID_HANDLE_VALUE) {
            break;
        }

        result = ui_msgbox_yesno(MBX_ANSI, "86Box", szMsg);

    } while (result != 0);

    return (dev->master_fd != (intptr_t) INVALID_HANDLE_VALUE) ? 1 : 0;
}

static int
open_pseudo_terminal(serial_passthrough_t *dev)
{
    char ascii_pipe_name[1024] = { 0 };
    strncpy(ascii_pipe_name, dev->named_pipe, sizeof(ascii_pipe_name));
    ascii_pipe_name[1023] = '\0';

    switch (dev->mode) {
        case SERPT_MODE_VCONSRV:
            if (!setup_pipe_server(dev, ascii_pipe_name)) {
                return 0;
            }
            pclog("Named Pipe Server @ %s\n", ascii_pipe_name);
            break;

        case SERPT_MODE_VCONCLNT:
            if (!connect_to_pipe_server(dev, ascii_pipe_name)) {
                fatal("Named pipe server not available (named_pipe=\"%hs\", port=COM%d)", ascii_pipe_name, dev->port + 1);
                return 0;
            }

            dev->ov_read_event = CreateEvent(NULL, TRUE, FALSE, NULL);
            memset(&dev->ov_read, 0, sizeof(dev->ov_read));
            dev->ov_read.hEvent  = dev->ov_read_event;
            dev->ov_read_pending = FALSE;

            dev->ov_write_event = CreateEvent(NULL, TRUE, FALSE, NULL);
            memset(&dev->ov_write, 0, sizeof(dev->ov_write));
            dev->ov_write.hEvent  = dev->ov_write_event;
            dev->ov_write_pending = FALSE;

            pclog("Named Pipe Client @ %s\n", ascii_pipe_name);
            break;

        default:
            // Invalid mode ...
            break;
    }

    return 1;
}

static int
open_host_serial_port(serial_passthrough_t *dev)
{
    COMMTIMEOUTS timeouts = {
        .ReadIntervalTimeout         = MAXDWORD,
        .ReadTotalTimeoutConstant    = 0,
        .ReadTotalTimeoutMultiplier  = 0,
        .WriteTotalTimeoutMultiplier = 0,
        .WriteTotalTimeoutConstant   = 1000
    };
    DCB *serialattr = calloc(1, sizeof(DCB));
    if (!serialattr)
        return 0;
    dev->master_fd = (intptr_t) CreateFileA(dev->host_serial_path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_WRITE_THROUGH, NULL);
    if (dev->master_fd == (intptr_t) INVALID_HANDLE_VALUE) {
        free(serialattr);
        return 0;
    }
    if (!SetCommTimeouts((HANDLE) dev->master_fd, &timeouts)) {
        pclog(LOG_PREFIX "error setting COM port timeouts.\n");
        CloseHandle((HANDLE) dev->master_fd);
        free(serialattr);
        return 0;
    }
    GetCommState((HANDLE) dev->master_fd, serialattr);
    dev->backend_priv = serialattr;
    return 1;
}

int
plat_serpt_open_device(void *priv)
{
    serial_passthrough_t *dev = (serial_passthrough_t *) priv;

    switch (dev->mode) {
        case SERPT_MODE_VCONSRV:
        case SERPT_MODE_VCONCLNT:
            if (open_pseudo_terminal(dev)) {

                //dev->ov_read_event = CreateEvent(NULL, TRUE, FALSE, NULL);
                //memset(&dev->ov_read, 0, sizeof(dev->ov_read));
                //dev->ov_read.hEvent  = dev->ov_read_event;
                //dev->ov_read_pending = FALSE;

                //dev->ov_write_event = CreateEvent(NULL, TRUE, FALSE, NULL);
                //memset(&dev->ov_write, 0, sizeof(dev->ov_write));
                //dev->ov_write.hEvent  = dev->ov_write_event;
                //dev->ov_write_pending = FALSE;

                return 0;
            }
            break;
        case SERPT_MODE_HOSTSER:
            if (open_host_serial_port(dev)) {
                return 0;
            }
        default:
            break;
    }
    return 1;
}
