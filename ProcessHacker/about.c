/*
 * Process Hacker -
 *   about dialog
 *
 * Copyright (C) 2010-2016 wj32
 * Copyright (C) 2017 dmex
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <phapp.h>

#include <windowsx.h>
#include <symprv.h>

#include <hndlprv.h>
#include <memprv.h>
#include <modprv.h>
#include <netprv.h>
#include <phappres.h>
#include <procprv.h>
#include <srvprv.h>
#include <thrdprv.h>

static INT_PTR CALLBACK PhpAboutDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PPH_STRING appName;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

#if (PHAPP_VERSION_REVISION != 0)
            appName = PhFormatString(
                L"Process Hacker %u.%u.%u",
                PHAPP_VERSION_MAJOR,
                PHAPP_VERSION_MINOR,
                PHAPP_VERSION_REVISION
                );
#else
            appName = PhFormatString(
                L"Process Hacker %u.%u",
                PHAPP_VERSION_MAJOR,
                PHAPP_VERSION_MINOR
                );
#endif

            SetDlgItemText(hwndDlg, IDC_ABOUT_NAME, appName->Buffer);
            PhDereferenceObject(appName);

            SetDlgItemText(hwndDlg, IDC_CREDITS,
                L"Thanks to:\n"
                L"    <a href=\"https://github.com/wj32\">wj32</a> - Wen Jia Liu\n"
                L"    <a href=\"https://github.com/dmex\">dmex</a> - Steven G\n"
                L"    <a href=\"https://github.com/xhmikosr\">XhmikosR</a>\n"
                L"    Donors - thank you for your support!\n\n"
                L"Process Hacker uses the following components:\n"
                L"    <a href=\"http://www.minixml.org\">Mini-XML</a> by Michael Sweet\n"
                L"    <a href=\"http://www.pcre.org\">PCRE</a>\n"
                L"    MD5 code by Jouni Malinen\n"
                L"    SHA1 code by Filip Navara, based on code by Steve Reid\n"
                L"    <a href=\"http://www.famfamfam.com/lab/icons/silk\">Silk icons</a>\n"
                L"    <a href=\"http://www.fatcow.com/free-icons\">Farm-fresh web icons</a>\n"
                );

            SendMessage(hwndDlg, WM_NEXTDLGCTL, (LPARAM)GetDlgItem(hwndDlg, IDOK), TRUE);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            case IDC_DIAGNOSTICS:
                {
                    PhShowInformationDialog(hwndDlg, PH_AUTO_T(PH_STRING, PhGetDiagnosticsString())->Buffer, 0);
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case NM_CLICK:
                {
                    switch (header->idFrom)
                    {
                    case IDC_CREDITS:
                    case IDC_LINK_SF:
                        PhShellExecute(hwndDlg, ((PNMLINK)header)->item.szUrl, NULL);
                        break;
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

VOID PhShowAboutDialog(
    VOID
    )
{
    DialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_ABOUT),
        NULL,
        PhpAboutDlgProc
        );
}

FORCEINLINE ULONG PhpGetObjectTypeObjectCount(
    _In_ PPH_OBJECT_TYPE ObjectType
    )
{
    PH_OBJECT_TYPE_INFORMATION info;

    PhGetObjectTypeInformation(ObjectType, &info);

    return info.NumberOfObjects;
}

PPH_STRING PhGetDiagnosticsString(
    VOID
    )
{
    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, 50);

    PhAppendFormatStringBuilder(&stringBuilder, L"OBJECT INFORMATION\r\n");

#define OBJECT_TYPE_COUNT(Type) PhAppendFormatStringBuilder(&stringBuilder, \
    L#Type L": %u objects\r\n", PhpGetObjectTypeObjectCount(Type))

    // ref
    OBJECT_TYPE_COUNT(PhObjectTypeObject);

    // basesup
    OBJECT_TYPE_COUNT(PhStringType);
    OBJECT_TYPE_COUNT(PhBytesType);
    OBJECT_TYPE_COUNT(PhListType);
    OBJECT_TYPE_COUNT(PhPointerListType);
    OBJECT_TYPE_COUNT(PhHashtableType);
    OBJECT_TYPE_COUNT(PhFileStreamType);

    // ph
    OBJECT_TYPE_COUNT(PhSymbolProviderType);
    OBJECT_TYPE_COUNT(PhProcessItemType);
    OBJECT_TYPE_COUNT(PhServiceItemType);
    OBJECT_TYPE_COUNT(PhNetworkItemType);
    OBJECT_TYPE_COUNT(PhModuleProviderType);
    OBJECT_TYPE_COUNT(PhModuleItemType);
    OBJECT_TYPE_COUNT(PhThreadProviderType);
    OBJECT_TYPE_COUNT(PhThreadItemType);
    OBJECT_TYPE_COUNT(PhHandleProviderType);
    OBJECT_TYPE_COUNT(PhHandleItemType);
    OBJECT_TYPE_COUNT(PhMemoryItemType);

    return PhFinalStringBuilderString(&stringBuilder);
}
