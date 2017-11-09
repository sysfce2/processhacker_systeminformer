/*
 * Process Hacker Window Explorer -
 *   main program
 *
 * Copyright (C) 2011 wj32
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

#include "wndexp.h"
#include "resource.h"

BOOLEAN IsHookClient;
PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginUnloadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
PH_CALLBACK_REGISTRATION MainMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessPropertiesInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ThreadMenuInitializingCallbackRegistration;

VOID NTAPI LoadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    NOTHING;
}

VOID NTAPI UnloadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    WeHookServerUninitialization();
}

BOOL CALLBACK WepEnumDesktopProc(
    _In_ LPTSTR lpszDesktop,
    _In_ LPARAM lParam
    )
{
    PhAddItemList((PPH_LIST)lParam, PhaCreateString(lpszDesktop)->Buffer);

    return TRUE;
}

VOID NTAPI MenuItemCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = Parameter;

    switch (menuItem->Id)
    {
    case ID_VIEW_WINDOWS:
        {
            WE_WINDOW_SELECTOR selector;

            selector.Type = WeWindowSelectorAll;
            WeShowWindowsDialog(WE_PhMainWndHandle, &selector);
        }
        break;
    case ID_VIEW_DESKTOPWINDOWS:
        {
            PPH_LIST desktopNames;
            PPH_STRING selectedChoice = NULL;

            desktopNames = PH_AUTO(PhCreateList(4));
            EnumDesktops(GetProcessWindowStation(), WepEnumDesktopProc, (LPARAM)desktopNames);

            if (PhaChoiceDialog(
                WE_PhMainWndHandle,
                L"Desktop Windows",
                L"Display windows for the following desktop:",
                (PWSTR *)desktopNames->Items,
                desktopNames->Count,
                NULL,
                PH_CHOICE_DIALOG_CHOICE,
                &selectedChoice,
                NULL,
                NULL
                ))
            {
                WE_WINDOW_SELECTOR selector;

                selector.Type = WeWindowSelectorDesktop;
                PhSetReference(&selector.Desktop.DesktopName, selectedChoice);
                WeShowWindowsDialog(WE_PhMainWndHandle, &selector);
            }
        }
        break;
    case ID_PROCESS_WINDOWS:
        {
            WE_WINDOW_SELECTOR selector;

            selector.Type = WeWindowSelectorProcess;
            selector.Process.ProcessId = ((PPH_PROCESS_ITEM)menuItem->Context)->ProcessId;
            WeShowWindowsDialog(WE_PhMainWndHandle, &selector);
        }
        break;
    case ID_THREAD_WINDOWS:
        {
            WE_WINDOW_SELECTOR selector;

            selector.Type = WeWindowSelectorThread;
            selector.Thread.ThreadId = ((PPH_THREAD_ITEM)menuItem->Context)->ThreadId;
            WeShowWindowsDialog(WE_PhMainWndHandle, &selector);
        }
        break;
    }
}

VOID NTAPI MainMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    ULONG insertIndex;
    PPH_EMENU_ITEM menuItem;
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;

    if (menuInfo->u.MainMenu.SubMenuIndex != PH_MENU_ITEM_LOCATION_VIEW)
        return;

    if (menuItem = PhFindEMenuItem(menuInfo->Menu, PH_EMENU_FIND_STARTSWITH, L"System Information", 0))
        insertIndex = PhIndexOfEMenuItem(menuInfo->Menu, menuItem) + 1;
    else
        insertIndex = 0;

    PhInsertEMenuItem(menuInfo->Menu, menuItem = PhPluginCreateEMenuItem(PluginInstance, 0, ID_VIEW_WINDOWS, L"Windows", NULL), insertIndex);

    if (PhGetIntegerSetting(SETTING_NAME_SHOW_DESKTOP_WINDOWS))
    {
        insertIndex = PhIndexOfEMenuItem(menuInfo->Menu, menuItem) + 1;

        PhInsertEMenuItem(menuInfo->Menu, PhPluginCreateEMenuItem(PluginInstance, 0, ID_VIEW_DESKTOPWINDOWS, L"Desktop Windows...", NULL), insertIndex);
    }
}

VOID NTAPI ProcessPropertiesInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_PROCESS_PROPCONTEXT propContext = Parameter;

    if (
        propContext->ProcessItem->ProcessId != SYSTEM_IDLE_PROCESS_ID && 
        propContext->ProcessItem->ProcessId != SYSTEM_PROCESS_ID
        )
    {
        WE_WINDOW_SELECTOR selector;

        selector.Type = WeWindowSelectorProcess;
        selector.Process.ProcessId = propContext->ProcessItem->ProcessId;
        WeShowWindowsPropPage(propContext, &selector);
    }
}

VOID NTAPI ThreadMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    PPH_THREAD_ITEM threadItem;
    ULONG insertIndex;
    PPH_EMENU_ITEM menuItem;

    if (menuInfo->u.Thread.NumberOfThreads == 1)
        threadItem = menuInfo->u.Thread.Threads[0];
    else
        threadItem = NULL;

    if (menuItem = PhFindEMenuItem(menuInfo->Menu, 0, L"Token", 0))
        insertIndex = PhIndexOfEMenuItem(menuInfo->Menu, menuItem) + 1;
    else
        insertIndex = 0;

    PhInsertEMenuItem(menuInfo->Menu, menuItem = PhPluginCreateEMenuItem(PluginInstance, 0, ID_THREAD_WINDOWS,
        L"Windows", threadItem), insertIndex);

    if (!threadItem) menuItem->Flags |= PH_EMENU_DISABLED;
}

LOGICAL DllMain(
    _In_ HINSTANCE Instance,
    _In_ ULONG Reason,
    _Reserved_ PVOID Reserved
    )
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        {
            BOOLEAN isClient;
            PPH_PLUGIN_INFORMATION info;
            PH_SETTING_CREATE settings[] =
            {
                { IntegerSettingType, SETTING_NAME_SHOW_DESKTOP_WINDOWS, L"0" },
                { StringSettingType, SETTING_NAME_WINDOW_TREE_LIST_COLUMNS, L"" },
                { IntegerPairSettingType, SETTING_NAME_WINDOWS_WINDOW_POSITION, L"100,100" },
                { ScalableIntegerPairSettingType, SETTING_NAME_WINDOWS_WINDOW_SIZE, L"@96|690,540" }
            };

            isClient = FALSE;

            if (!GetModuleHandle(L"ProcessHacker.exe") || !WeGetProcedureAddress("PhInstanceHandle"))
            {
                isClient = TRUE;
            }
            else
            {
                // WindowExplorer appears to be loading within Process Hacker. However, if there is
                // already a server instance, the the hook will be active, and our DllMain routine
                // will most likely be called before the plugin system is even initialized. Attempting
                // to register a plugin would result in an access violation, so load as a client for now.
                if (WeIsServerActive())
                    isClient = TRUE;
            }

            if (isClient)
            {
                // This DLL is being loaded not as a Process Hacker plugin, but as a hook.
                IsHookClient = TRUE;
                WeHookClientInitialization();

                break;
            }

            PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Window Explorer";
            info->Author = L"wj32";
            info->Description = L"View and manipulate windows.";
            info->Url = L"https://wj32.org/processhacker/forums/viewtopic.php?t=1116";
            info->HasOptions = FALSE;

            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackLoad),
                LoadCallback,
                NULL,
                &PluginLoadCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackUnload),
                UnloadCallback,
                NULL,
                &PluginUnloadCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackMenuItem),
                MenuItemCallback,
                NULL,
                &PluginMenuItemCallbackRegistration
                );

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackMainMenuInitializing),
                MainMenuInitializingCallback,
                NULL,
                &MainMenuInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessPropertiesInitializing),
                ProcessPropertiesInitializingCallback,
                NULL,
                &ProcessPropertiesInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackThreadMenuInitializing),
                ThreadMenuInitializingCallback,
                NULL,
                &ThreadMenuInitializingCallbackRegistration
                );

            PhAddSettings(settings, ARRAYSIZE(settings));    
        }
        break;
    case DLL_PROCESS_DETACH:
        {
            if (IsHookClient)
            {
                WeHookClientUninitialization();
            }
        }
        break;
    }

    return TRUE;
}
