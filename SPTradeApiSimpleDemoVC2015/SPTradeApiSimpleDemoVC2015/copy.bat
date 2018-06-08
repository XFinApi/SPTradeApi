xcopy ..\..\TradeApi_win32x86\Api\SPTradeApi_R8.75.4 ..\Release\TradeApi_win32x86\Api\SPTradeApi_R8.75.4 /I /E /Y
copy ..\..\TradeApi_win32x86\Cpp\XFinApi.ITradeApi.dll ..\Release\XFinApi.ITradeApi.dll /Y

xcopy ..\..\TradeApi_win32x86\Api\SPTradeApi_R8.75.4 ..\Debug\TradeApi_win32x86\Api\SPTradeApi_R8.75.4 /I /E /Y
copy ..\..\TradeApi_win32x86\Cpp\XFinApi.ITradeApid.dll ..\Debug\XFinApi.ITradeApid.dll /Y

pause