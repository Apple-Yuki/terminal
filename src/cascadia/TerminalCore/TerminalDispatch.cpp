// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include "TerminalDispatch.hpp"
#include "../../types/inc/utils.hpp"

using namespace Microsoft::Console;
using namespace ::Microsoft::Terminal::Core;
using namespace ::Microsoft::Console::Render;
using namespace ::Microsoft::Console::VirtualTerminal;

// NOTE:
// Functions related to Set Graphics Renditions (SGR) are in
//      TerminalDispatchGraphics.cpp, not this file

TerminalDispatch::TerminalDispatch(ITerminalApi& terminalApi) noexcept :
    _terminalApi{ terminalApi }
{
}

void TerminalDispatch::Print(const wchar_t wchPrintable)
{
    _terminalApi.PrintString({ &wchPrintable, 1 });
}

void TerminalDispatch::PrintString(const std::wstring_view string)
{
    _terminalApi.PrintString(string);
}

bool TerminalDispatch::CursorPosition(VTInt line,
                                      VTInt column)
{
    _terminalApi.SetCursorPosition({ column - 1, line - 1 });
    return true;
}

bool TerminalDispatch::CursorVisibility(const bool isVisible)
{
    _terminalApi.SetCursorVisibility(isVisible);
    return true;
}

bool TerminalDispatch::EnableCursorBlinking(const bool enable)
{
    _terminalApi.EnableCursorBlinking(enable);
    return true;
}

bool TerminalDispatch::CursorForward(const VTInt distance)
{
    const auto cursorPos = _terminalApi.GetCursorPosition();
    _terminalApi.SetCursorPosition({ cursorPos.X + distance, cursorPos.Y });
    return true;
}

bool TerminalDispatch::CursorBackward(const VTInt distance)
{
    const auto cursorPos = _terminalApi.GetCursorPosition();
    _terminalApi.SetCursorPosition({ cursorPos.X - distance, cursorPos.Y });
    return true;
}

bool TerminalDispatch::CursorUp(const VTInt distance)
{
    const auto cursorPos = _terminalApi.GetCursorPosition();
    _terminalApi.SetCursorPosition({ cursorPos.X, cursorPos.Y + distance });
    return true;
}

bool TerminalDispatch::LineFeed(const DispatchTypes::LineFeedType lineFeedType)
{
    switch (lineFeedType)
    {
    case DispatchTypes::LineFeedType::DependsOnMode:
        // There is currently no need for mode-specific line feeds in the Terminal,
        // so for now we just treat them as a line feed without carriage return.
    case DispatchTypes::LineFeedType::WithoutReturn:
        _terminalApi.CursorLineFeed(false);
        return true;
    case DispatchTypes::LineFeedType::WithReturn:
        _terminalApi.CursorLineFeed(true);
        return true;
    default:
        return false;
    }
}

bool TerminalDispatch::EraseCharacters(const VTInt numChars)
{
    _terminalApi.EraseCharacters(numChars);
    return true;
}

bool TerminalDispatch::WarningBell()
{
    _terminalApi.WarningBell();
    return true;
}

bool TerminalDispatch::CarriageReturn()
{
    const auto cursorPos = _terminalApi.GetCursorPosition();
    _terminalApi.SetCursorPosition({ 0, cursorPos.Y });
    return true;
}

bool TerminalDispatch::SetWindowTitle(std::wstring_view title)
{
    _terminalApi.SetWindowTitle(title);
    return true;
}

bool TerminalDispatch::HorizontalTabSet()
{
    const auto width = _terminalApi.GetBufferSize().Dimensions().X;
    const auto column = _terminalApi.GetCursorPosition().X;

    _InitTabStopsForWidth(width);
    _tabStopColumns.at(column) = true;
    return true;
}

bool TerminalDispatch::ForwardTab(const VTInt numTabs)
{
    const auto width = _terminalApi.GetBufferSize().Dimensions().X;
    const auto cursorPosition = _terminalApi.GetCursorPosition();
    auto column = cursorPosition.X;
    const auto row = cursorPosition.Y;
    VTInt tabsPerformed = 0;
    _InitTabStopsForWidth(width);
    while (column + 1 < width && tabsPerformed < numTabs)
    {
        column++;
        if (til::at(_tabStopColumns, column))
        {
            tabsPerformed++;
        }
    }

    _terminalApi.SetCursorPosition({ column, row });
    return true;
}

bool TerminalDispatch::BackwardsTab(const VTInt numTabs)
{
    const auto width = _terminalApi.GetBufferSize().Dimensions().X;
    const auto cursorPosition = _terminalApi.GetCursorPosition();
    auto column = cursorPosition.X;
    const auto row = cursorPosition.Y;
    VTInt tabsPerformed = 0;
    _InitTabStopsForWidth(width);
    while (column > 0 && tabsPerformed < numTabs)
    {
        column--;
        if (til::at(_tabStopColumns, column))
        {
            tabsPerformed++;
        }
    }

    _terminalApi.SetCursorPosition({ column, row });
    return true;
}

bool TerminalDispatch::TabClear(const DispatchTypes::TabClearType clearType)
{
    switch (clearType)
    {
    case DispatchTypes::TabClearType::ClearCurrentColumn:
        _ClearSingleTabStop();
        return true;
    case DispatchTypes::TabClearType::ClearAllColumns:
        _ClearAllTabStops();
        return true;
    default:
        return false;
    }
}

// Method Description:
// - Sets a single entry of the colortable to a new value
// Arguments:
// - tableIndex: The VT color table index
// - color: The new RGB color value to use.
// Return Value:
// - True.
bool TerminalDispatch::SetColorTableEntry(const size_t tableIndex,
                                          const DWORD color)
{
    _terminalApi.SetColorTableEntry(tableIndex, color);
    return true;
}

bool TerminalDispatch::SetCursorStyle(const DispatchTypes::CursorStyle cursorStyle)
{
    _terminalApi.SetCursorStyle(cursorStyle);
    return true;
}

bool TerminalDispatch::SetCursorColor(const DWORD color)
{
    _terminalApi.SetColorTableEntry(TextColor::CURSOR_COLOR, color);
    return true;
}

bool TerminalDispatch::SetClipboard(std::wstring_view content)
{
    _terminalApi.CopyToClipboard(content);
    return true;
}

// Method Description:
// - Sets the default foreground color to a new value
// Arguments:
// - color: The new RGB color value to use, in 0x00BBGGRR form
// Return Value:
// - True.
bool TerminalDispatch::SetDefaultForeground(const DWORD color)
{
    _terminalApi.SetColorAliasIndex(ColorAlias::DefaultForeground, TextColor::DEFAULT_FOREGROUND);
    _terminalApi.SetColorTableEntry(TextColor::DEFAULT_FOREGROUND, color);
    return true;
}

// Method Description:
// - Sets the default background color to a new value
// Arguments:
// - color: The new RGB color value to use, in 0x00BBGGRR form
// Return Value:
// - True.
bool TerminalDispatch::SetDefaultBackground(const DWORD color)
{
    _terminalApi.SetColorAliasIndex(ColorAlias::DefaultBackground, TextColor::DEFAULT_BACKGROUND);
    _terminalApi.SetColorTableEntry(TextColor::DEFAULT_BACKGROUND, color);
    return true;
}

// Method Description:
// - Erases characters in the buffer depending on the erase type
// Arguments:
// - eraseType: the erase type (from beginning, to end, or all)
// Return Value:
// True if handled successfully. False otherwise.
bool TerminalDispatch::EraseInLine(const DispatchTypes::EraseType eraseType)
{
    return _terminalApi.EraseInLine(eraseType);
}

// Method Description:
// - Deletes count number of characters starting from where the cursor is currently
// Arguments:
// - count, the number of characters to delete
// Return Value:
// - True.
bool TerminalDispatch::DeleteCharacter(const VTInt count)
{
    _terminalApi.DeleteCharacter(count);
    return true;
}

// Method Description:
// - Adds count number of spaces starting from where the cursor is currently
// Arguments:
// - count, the number of spaces to add
// Return Value:
// - True.
bool TerminalDispatch::InsertCharacter(const VTInt count)
{
    _terminalApi.InsertCharacter(count);
    return true;
}

// Method Description:
// - Moves the viewport and erases text from the buffer depending on the eraseType
// Arguments:
// - eraseType: the desired erase type
// Return Value:
// True if handled successfully. False otherwise
bool TerminalDispatch::EraseInDisplay(const DispatchTypes::EraseType eraseType)
{
    return _terminalApi.EraseInDisplay(eraseType);
}

// - DECKPAM, DECKPNM - Sets the keypad input mode to either Application mode or Numeric mode (true, false respectively)
// Arguments:
// - applicationMode - set to true to enable Application Mode Input, false for Numeric Mode Input.
// Return Value:
// - True.
bool TerminalDispatch::SetKeypadMode(const bool applicationMode)
{
    _terminalApi.SetInputMode(TerminalInput::Mode::Keypad, applicationMode);
    return true;
}

// - DECCKM - Sets the cursor keys input mode to either Application mode or Normal mode (true, false respectively)
// Arguments:
// - applicationMode - set to true to enable Application Mode Input, false for Normal Mode Input.
// Return Value:
// - True.
bool TerminalDispatch::SetCursorKeysMode(const bool applicationMode)
{
    _terminalApi.SetInputMode(TerminalInput::Mode::CursorKey, applicationMode);
    return true;
}

// Routine Description:
// - DECSCNM - Sets the screen mode to either normal or reverse.
//    When in reverse screen mode, the background and foreground colors are switched.
// Arguments:
// - reverseMode - set to true to enable reverse screen mode, false for normal mode.
// Return Value:
// - True.
bool TerminalDispatch::SetScreenMode(const bool reverseMode)
{
    _terminalApi.SetRenderMode(RenderSettings::Mode::ScreenReversed, reverseMode);
    return true;
}

// Method Description:
// - win32-input-mode: Enable sending full input records encoded as a string of
//   characters to the client application.
// Arguments:
// - win32InputMode - set to true to enable win32-input-mode, false to disable.
// Return Value:
// - True.
bool TerminalDispatch::EnableWin32InputMode(const bool win32Mode)
{
    _terminalApi.SetInputMode(TerminalInput::Mode::Win32, win32Mode);
    return true;
}

//Routine Description:
// Enable VT200 Mouse Mode - Enables/disables the mouse input handler in default tracking mode.
//Arguments:
// - enabled - true to enable, false to disable.
// Return value:
// - True.
bool TerminalDispatch::EnableVT200MouseMode(const bool enabled)
{
    _terminalApi.SetInputMode(TerminalInput::Mode::DefaultMouseTracking, enabled);
    return true;
}

//Routine Description:
// Enable UTF-8 Extended Encoding - this changes the encoding scheme for sequences
//      emitted by the mouse input handler. Does not enable/disable mouse mode on its own.
//Arguments:
// - enabled - true to enable, false to disable.
// Return value:
// - True.
bool TerminalDispatch::EnableUTF8ExtendedMouseMode(const bool enabled)
{
    _terminalApi.SetInputMode(TerminalInput::Mode::Utf8MouseEncoding, enabled);
    return true;
}

//Routine Description:
// Enable SGR Extended Encoding - this changes the encoding scheme for sequences
//      emitted by the mouse input handler. Does not enable/disable mouse mode on its own.
//Arguments:
// - enabled - true to enable, false to disable.
// Return value:
// - True.
bool TerminalDispatch::EnableSGRExtendedMouseMode(const bool enabled)
{
    _terminalApi.SetInputMode(TerminalInput::Mode::SgrMouseEncoding, enabled);
    return true;
}

//Routine Description:
// Enable Button Event mode - send mouse move events WITH A BUTTON PRESSED to the input.
//Arguments:
// - enabled - true to enable, false to disable.
// Return value:
// - True.
bool TerminalDispatch::EnableButtonEventMouseMode(const bool enabled)
{
    _terminalApi.SetInputMode(TerminalInput::Mode::ButtonEventMouseTracking, enabled);
    return true;
}

//Routine Description:
// Enable Any Event mode - send all mouse events to the input.
//Arguments:
// - enabled - true to enable, false to disable.
// Return value:
// - True.
bool TerminalDispatch::EnableAnyEventMouseMode(const bool enabled)
{
    _terminalApi.SetInputMode(TerminalInput::Mode::AnyEventMouseTracking, enabled);
    return true;
}

// Method Description:
// - Enables/disables focus event mode. A client may enable this if they want to
//   receive focus events.
// - ConPTY always enables this mode and never disables it. For more details, see GH#12900.
// Arguments:
// - enabled - true to enable, false to disable.
// Return Value:
// - True if handled successfully. False otherwise.
bool TerminalDispatch::EnableFocusEventMode(const bool enabled)
{
    _terminalApi.SetInputMode(TerminalInput::Mode::FocusEvent, enabled);
    return true;
}

//Routine Description:
// Enable Alternate Scroll Mode - When in the Alt Buffer, send CUP and CUD on
//      scroll up/down events instead of the usual sequences
//Arguments:
// - enabled - true to enable, false to disable.
// Return value:
// - True.
bool TerminalDispatch::EnableAlternateScroll(const bool enabled)
{
    _terminalApi.SetInputMode(TerminalInput::Mode::AlternateScroll, enabled);
    return true;
}

//Routine Description:
// Enable Bracketed Paste Mode -  this changes the behavior of pasting.
//      See: https://www.xfree86.org/current/ctlseqs.html#Bracketed%20Paste%20Mode
//Arguments:
// - enabled - true to enable, false to disable.
// Return value:
// - True.
bool TerminalDispatch::EnableXtermBracketedPasteMode(const bool enabled)
{
    _terminalApi.EnableXtermBracketedPasteMode(enabled);
    return true;
}

bool TerminalDispatch::SetMode(const DispatchTypes::ModeParams param)
{
    return _ModeParamsHelper(param, true);
}

bool TerminalDispatch::ResetMode(const DispatchTypes::ModeParams param)
{
    return _ModeParamsHelper(param, false);
}

// Routine Description:
// - DSR - Reports status of a console property back to the STDIN based on the type of status requested.
//       - This particular routine responds to ANSI status patterns only (CSI # n), not the DEC format (CSI ? # n)
// Arguments:
// - statusType - ANSI status type indicating what property we should report back
// Return Value:
// - True if handled successfully. False otherwise.
bool TerminalDispatch::DeviceStatusReport(const DispatchTypes::AnsiStatusType statusType)
{
    auto success = false;

    switch (statusType)
    {
    case DispatchTypes::AnsiStatusType::OS_OperatingStatus:
        success = _OperatingStatus();
        break;
    case DispatchTypes::AnsiStatusType::CPR_CursorPositionReport:
        success = _CursorPositionReport();
        break;
    }

    return success;
}

// Routine Description:
// - DSR-OS - Reports the operating status back to the input channel
// Arguments:
// - <none>
// Return Value:
// - True if handled successfully. False otherwise.
bool TerminalDispatch::_OperatingStatus() const
{
    // We always report a good operating condition.
    return _WriteResponse(L"\x1b[0n");
}

// Routine Description:
// - DSR-CPR - Reports the current cursor position within the viewport back to the input channel
// Arguments:
// - <none>
// Return Value:
// - True if handled successfully. False otherwise.
bool TerminalDispatch::_CursorPositionReport() const
{
    // Now send it back into the input channel of the console.
    // First format the response string.
    const auto pos = _terminalApi.GetCursorPosition();
    // VT has origin at 1,1 where as we use 0,0 internally
    const auto response = wil::str_printf<std::wstring>(L"\x1b[%d;%dR", pos.Y + 1, pos.X + 1);
    return _WriteResponse(response);
}

// Method Description:
// - Start a hyperlink
// Arguments:
// - uri - the hyperlink URI
// - params - the optional custom ID
// Return Value:
// - true
bool TerminalDispatch::AddHyperlink(const std::wstring_view uri, const std::wstring_view params)
{
    _terminalApi.AddHyperlink(uri, params);
    return true;
}

// Method Description:
// - End a hyperlink
// Return Value:
// - true
bool TerminalDispatch::EndHyperlink()
{
    _terminalApi.EndHyperlink();
    return true;
}

// Method Description:
// - Performs a ConEmu action
// - Currently, the only action we support is setting the taskbar state/progress
// Arguments:
// - string: contains the parameters that define which action we do
// Return Value:
// - true
bool TerminalDispatch::DoConEmuAction(const std::wstring_view string)
{
    unsigned int state = 0;
    unsigned int progress = 0;

    const auto parts = Utils::SplitString(string, L';');
    unsigned int subParam = 0;

    if (parts.size() < 1 || !Utils::StringToUint(til::at(parts, 0), subParam))
    {
        return false;
    }

    // 4 is SetProgressBar, which sets the taskbar state/progress.
    if (subParam == 4)
    {
        if (parts.size() >= 2)
        {
            // A state parameter is defined, parse it out
            const auto stateSuccess = Utils::StringToUint(til::at(parts, 1), state);
            if (!stateSuccess && !til::at(parts, 1).empty())
            {
                return false;
            }
            if (parts.size() >= 3)
            {
                // A progress parameter is also defined, parse it out
                const auto progressSuccess = Utils::StringToUint(til::at(parts, 2), progress);
                if (!progressSuccess && !til::at(parts, 2).empty())
                {
                    return false;
                }
            }
        }

        if (state > TaskbarMaxState)
        {
            // state is out of bounds, return false
            return false;
        }
        if (progress > TaskbarMaxProgress)
        {
            // progress is greater than the maximum allowed value, clamp it to the max
            progress = TaskbarMaxProgress;
        }
        _terminalApi.SetTaskbarProgress(static_cast<DispatchTypes::TaskbarState>(state), progress);
        return true;
    }
    // 9 is SetWorkingDirectory, which informs the terminal about the current working directory.
    else if (subParam == 9)
    {
        if (parts.size() >= 2)
        {
            const auto path = til::at(parts, 1);
            // The path should be surrounded with '"' according to the documentation of ConEmu.
            // An example: 9;"D:/"
            if (path.at(0) == L'"' && path.at(path.size() - 1) == L'"' && path.size() >= 3)
            {
                _terminalApi.SetWorkingDirectory(path.substr(1, path.size() - 2));
            }
            else
            {
                // If we fail to find the surrounding quotation marks, we'll give the path a try anyway.
                // ConEmu also does this.
                _terminalApi.SetWorkingDirectory(path);
            }
            return true;
        }
    }

    return false;
}

// Routine Description:
// - Helper to send a string reply to the input stream of the console.
// - Used by various commands where the program attached would like a reply to one of the commands issued.
// Arguments:
// - reply - The reply string to transmit back to the input stream
// Return Value:
// - True if the string was sent to the connected application. False otherwise.
bool TerminalDispatch::_WriteResponse(const std::wstring_view reply) const
{
    return _terminalApi.ReturnResponse(reply);
}

// Routine Description:
// - Support routine for routing private mode parameters to be set/reset as flags
// Arguments:
// - param - mode parameter to set/reset
// - enable - True for set, false for unset.
// Return Value:
// - True if handled successfully. False otherwise.
bool TerminalDispatch::_ModeParamsHelper(const DispatchTypes::ModeParams param, const bool enable)
{
    auto success = false;
    switch (param)
    {
    case DispatchTypes::ModeParams::DECCKM_CursorKeysMode:
        // set - Enable Application Mode, reset - Normal mode
        success = SetCursorKeysMode(enable);
        break;
    case DispatchTypes::ModeParams::DECSCNM_ScreenMode:
        success = SetScreenMode(enable);
        break;
    case DispatchTypes::ModeParams::VT200_MOUSE_MODE:
        success = EnableVT200MouseMode(enable);
        break;
    case DispatchTypes::ModeParams::BUTTON_EVENT_MOUSE_MODE:
        success = EnableButtonEventMouseMode(enable);
        break;
    case DispatchTypes::ModeParams::ANY_EVENT_MOUSE_MODE:
        success = EnableAnyEventMouseMode(enable);
        break;
    case DispatchTypes::ModeParams::UTF8_EXTENDED_MODE:
        success = EnableUTF8ExtendedMouseMode(enable);
        break;
    case DispatchTypes::ModeParams::SGR_EXTENDED_MODE:
        success = EnableSGRExtendedMouseMode(enable);
        break;
    case DispatchTypes::ModeParams::FOCUS_EVENT_MODE:
        success = EnableFocusEventMode(enable);
        break;
    case DispatchTypes::ModeParams::ALTERNATE_SCROLL:
        success = EnableAlternateScroll(enable);
        break;
    case DispatchTypes::ModeParams::DECTCEM_TextCursorEnableMode:
        success = CursorVisibility(enable);
        break;
    case DispatchTypes::ModeParams::ATT610_StartCursorBlink:
        success = EnableCursorBlinking(enable);
        break;
    case DispatchTypes::ModeParams::XTERM_BracketedPasteMode:
        success = EnableXtermBracketedPasteMode(enable);
        break;
    case DispatchTypes::ModeParams::W32IM_Win32InputMode:
        success = EnableWin32InputMode(enable);
        break;
    case DispatchTypes::ModeParams::ASB_AlternateScreenBuffer:
        success = enable ? UseAlternateScreenBuffer() : UseMainScreenBuffer();
        break;
    default:
        // If no functions to call, overall dispatch was a failure.
        success = false;
        break;
    }
    return success;
}

void TerminalDispatch::_ClearSingleTabStop()
{
    const auto width = _terminalApi.GetBufferSize().Dimensions().X;
    const auto column = _terminalApi.GetCursorPosition().X;

    _InitTabStopsForWidth(width);
    _tabStopColumns.at(column) = false;
}

void TerminalDispatch::_ClearAllTabStops()
{
    _tabStopColumns.clear();
    _initDefaultTabStops = false;
}

void TerminalDispatch::_ResetTabStops()
{
    _tabStopColumns.clear();
    _initDefaultTabStops = true;
}

void TerminalDispatch::_InitTabStopsForWidth(const size_t width)
{
    const auto initialWidth = _tabStopColumns.size();
    if (width > initialWidth)
    {
        _tabStopColumns.resize(width);
        if (_initDefaultTabStops)
        {
            for (auto column = 8u; column < _tabStopColumns.size(); column += 8)
            {
                if (column >= initialWidth)
                {
                    til::at(_tabStopColumns, column) = true;
                }
            }
        }
    }
}

bool TerminalDispatch::SoftReset()
{
    // TODO:GH#1883 much of this method is not yet implemented in the Terminal,
    // because the Terminal _doesn't need to_ yet. The terminal is only ever
    // connected to conpty, so it doesn't implement most of these things that
    // Hard/Soft Reset would reset. As those things are implemented, they should

    // also get cleared here.
    //
    // This code is left here (from its original form in conhost) as a reminder
    // of what needs to be done.

    CursorVisibility(true); // Cursor enabled.
    // SetOriginMode(false); // Absolute cursor addressing.
    // SetAutoWrapMode(true); // Wrap at end of line.
    SetCursorKeysMode(false); // Normal characters.
    SetKeypadMode(false); // Numeric characters.

    // // Top margin = 1; bottom margin = page length.
    // _DoSetTopBottomScrollingMargins(0, 0);

    // _termOutput = {}; // Reset all character set designations.
    // if (_initialCodePage.has_value())
    // {
    //     // Restore initial code page if previously changed by a DOCS sequence.
    //     _pConApi->SetConsoleOutputCP(_initialCodePage.value());
    // }

    SetGraphicsRendition({}); // Normal rendition.

    // // Reset the saved cursor state.
    // // Note that XTerm only resets the main buffer state, but that
    // // seems likely to be a bug. Most other terminals reset both.
    // _savedCursorState.at(0) = {}; // Main buffer
    // _savedCursorState.at(1) = {}; // Alt buffer

    return true;
}

bool TerminalDispatch::HardReset()
{
    // TODO:GH#1883 much of this method is not yet implemented in the Terminal,
    // because the Terminal _doesn't need to_ yet. The terminal is only ever
    // connected to conpty, so it doesn't implement most of these things that
    // Hard/Soft Reset would reset. As those things ar implemented, they should
    // also get cleared here.
    //
    // This code is left here (from its original form in conhost) as a reminder
    // of what needs to be done.

    // // If in the alt buffer, switch back to main before doing anything else.
    // if (_usingAltBuffer)
    // {
    //     _pConApi->PrivateUseMainScreenBuffer();
    //     _usingAltBuffer = false;
    // }

    // Sets the SGR state to normal - this must be done before EraseInDisplay
    //      to ensure that it clears with the default background color.
    SoftReset();

    // Clears the screen - Needs to be done in two operations.
    EraseInDisplay(DispatchTypes::EraseType::All);
    EraseInDisplay(DispatchTypes::EraseType::Scrollback);

    // Set the DECSCNM screen mode back to normal.
    SetScreenMode(false);

    // Cursor to 1,1 - the Soft Reset guarantees this is absolute
    CursorPosition(1, 1);

    // Reset the mouse mode
    EnableSGRExtendedMouseMode(false);
    EnableAnyEventMouseMode(false);

    // Delete all current tab stops and reapply
    _ResetTabStops();

    return true;
}

bool TerminalDispatch::WindowManipulation(const DispatchTypes::WindowManipulationType function,
                                          const VTParameter /*parameter1*/,
                                          const VTParameter /*parameter2*/)
{
    switch (function)
    {
    case DispatchTypes::WindowManipulationType::DeIconifyWindow:
        _terminalApi.ShowWindow(true);
        return true;
    case DispatchTypes::WindowManipulationType::IconifyWindow:
        _terminalApi.ShowWindow(false);
        return true;
    default:
        return false;
    }
}

// Routine Description:
// - DECSC - Saves the current "cursor state" into a memory buffer. This
//   includes the cursor position, origin mode, graphic rendition, and
//   active character set.
// Arguments:
// - <none>
// Return Value:
// - True.
bool TerminalDispatch::CursorSaveState()
{
    // TODO GH#3849: When de-duplicating this, the AdaptDispatch version of this
    // is more elaborate.
    const auto attributes = _terminalApi.GetTextAttributes();
    const auto coordCursor = _terminalApi.GetCursorPosition();
    // The cursor is given to us by the API as relative to current viewport top.

    // VT is also 1 based, not 0 based, so correct by 1.
    auto& savedCursorState = _savedCursorState.at(_usingAltBuffer);
    savedCursorState.Column = coordCursor.X + 1;
    savedCursorState.Row = coordCursor.Y + 1;
    savedCursorState.Attributes = attributes;

    return true;
}

// Routine Description:
// - DECRC - Restores a saved "cursor state" from the DECSC command back into
//   the console state. This includes the cursor position, origin mode, graphic
//   rendition, and active character set.
// Arguments:
// - <none>
// Return Value:
// - True.
bool TerminalDispatch::CursorRestoreState()
{
    // TODO GH#3849: When de-duplicating this, the AdaptDispatch version of this
    // is more elaborate.
    auto& savedCursorState = _savedCursorState.at(_usingAltBuffer);

    auto row = savedCursorState.Row;
    const auto col = savedCursorState.Column;

    // The saved coordinates are always absolute, so we need reset the origin mode temporarily.
    CursorPosition(row, col);

    // Restore text attributes.
    _terminalApi.SetTextAttributes(savedCursorState.Attributes);

    return true;
}

// - ASBSET - Creates and swaps to the alternate screen buffer. In virtual terminals, there exists both a "main"
//     screen buffer and an alternate. ASBSET creates a new alternate, and switches to it. If there is an already
//     existing alternate, it is discarded.
// Arguments:
// - None
// Return Value:
// - True.
bool TerminalDispatch::UseAlternateScreenBuffer()
{
    CursorSaveState();
    _terminalApi.UseAlternateScreenBuffer();
    _usingAltBuffer = true;
    return true;
}

// Routine Description:
// - ASBRST - From the alternate buffer, returns to the main screen buffer.
//     From the main screen buffer, does nothing. The alternate is discarded.
// Arguments:
// - None
// Return Value:
// - True.
bool TerminalDispatch::UseMainScreenBuffer()
{
    _terminalApi.UseMainScreenBuffer();
    _usingAltBuffer = false;
    CursorRestoreState();
    return true;
}
