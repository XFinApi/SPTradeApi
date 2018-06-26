/*******************************************************
* SPTradeApiSimpleDemoVC2015
* www.xfinapi.com
*******************************************************/

/*
VC++2015工程配置

1、包含目录
选择工程-属性-VC++目录-包含目录
添加：$(Solutiondir)..\TradeApi_win32x86\Cpp

2、lib库目录
选择工程-属性-VC++目录-库目录
添加：$(Solutiondir)..\TradeApi_win32x86\Cpp

3、lib依赖项
选择工程-属性-链接器-输入-附加依赖项
添加：
release使用：XFinApi.ITradeApi.lib
或
debug使用：XFinApi.ITradeApid.lib

4、调试工作目录
选择工程-属性-调试-工作目录
修改为：$(TargetDir)

5、拷贝库文件
参考copy.bat手动拷贝
或
运行时自动拷贝：
选择工程-属性-生成事件-后期生成事件-命令行，添加：copy.bat
*/

/*
开发完成的程序发布

1.拷贝XFinApi.ITradeApi.dll到可执行文件目录

2.拷贝TradeApi_win32x86\Api\SPTradeApi_RX.XX.X目录（X.XX.X为版本号）下的所有dll文件到可执行文件目录，
建议保留dll文件所处目录结构，解决接口dll文件可能重名的问题。
*/

#include <iostream>
#include <algorithm>
#include <thread>

#include "XFinApi.h"

//////////////////////////////////////////////////////////////////////////////////
//配置信息
class Config
{
public:
	//地址
	std::string HostAddress;

	//账户
	std::string UserName;
	std::string Password;

	std::string AppID;
	std::string License;

	//合约
	std::string InstrumentID;
	std::string ProductID;

	//行情
	double SellPrice1 = -1;
	double BuyPrice1 = -1;

	Config()
	{
		//注册SP仿真交易账号，http://demo.spsystem.info:8000/DemoAcc/DemoAcc_API.php?lang=2

		HostAddress = "demo.spsystem.info:8080";
		UserName = "DEMO201806119A";//公用测试账户。为了测试准确，请注册使用您自己的账户。
		Password = "a123456";

		AppID = "SPDEMO";
		License = "5B20847848D9D";

		InstrumentID = "SIZ8";
		ProductID = "SI";
	}
};

class MarketEvent;
class TradeEvent;

//////////////////////////////////////////////////////////////////////////////////
static Config Cfg;
static XFinApi::TradeApi::IMarket *market = nullptr;
static XFinApi::TradeApi::ITrade *trade = nullptr;
static MarketEvent *marketEvent = nullptr;
static TradeEvent *tradeEvent = nullptr;

//////////////////////////////////////////////////////////////////////////////////
//辅助方法
#define DEFAULT_FILTER(_x)  ( XFinApi::TradeApi::IsDefaultValue(_x) ? -1 : _x)

static void PrintNotifyInfo(const XFinApi::TradeApi::NotifyParams &param)
{
	std::string strs;
	for (const XFinApi::TradeApi::CodeInfo &info : param.CodeInfos)
	{
		strs += "(Code=" + info.Code +
			";LowerCode=" + info.LowerCode +
			";LowerMessage=" + info.LowerMessage + ")";
	}
	printf(" OnNotify: Action=%d, Result=%d%s\n",
		param.Action,
		param.Result,
		strs.c_str());
}

static void PrintSubscribedInfo(const XFinApi::TradeApi::QueryParams &instInfo)
{
	printf("- OnSubscribed: %s\n", instInfo.InstrumentID.c_str());
}

static void PrintUnsubscribedInfo(const XFinApi::TradeApi::QueryParams &instInfo)
{
	printf("- OnUnsubscribed: %s\n", instInfo.InstrumentID.c_str());
}

static void PrintTickInfo(const XFinApi::TradeApi::Tick &tick)
{
	printf("  Tick,%s %s, HighestPrice=%g, LowestPrice=%g, BidPrice0=%g, BidVolume0=%lld, AskPrice0=%g, AskVolume0=%lld, LastPrice=%g, LastVolume=%lld, TradingDay=%s, TradingTime=%s\n",
		tick.ExchangeID.c_str(),
		tick.InstrumentID.c_str(),
		tick.HighestPrice,
		tick.LowestPrice,
		tick.BidPrice[0],
		tick.BidVolume[0],
		tick.AskPrice[0],
		tick.AskVolume[0],
		tick.LastPrice,
		tick.LastVolume,
		tick.TradingDay.c_str(),
		tick.TradingTime.c_str());
}

static void  PrintOrderInfo(const XFinApi::TradeApi::Order &order)
{
	printf("  ProductType=%d, ID=%s, InstID=%s, Price=%g, Volume=%ld, NoTradedVolume=%ld, Direction=%d, OpenCloseType=%d, PriceCond=%d, TimeCond=%d, VolumeCond=%d, Status=%d, Msg=%s, %s\n",
		order.ProductType,
		order.OrderID.c_str(),
		order.InstrumentID.c_str(), order.Price, order.Volume, order.NoTradedVolume,
		order.Direction, order.OpenCloseType,
		order.PriceCond,
		order.TimeCond,
		order.VolumeCond,
		order.Status,
		order.StatusMsg.c_str(),
		order.OrderTime.c_str()
	);
}

static void  PrintTradeInfo(const XFinApi::TradeApi::TradeOrder &trade)
{
	printf("  ID=%s, OrderID=%s, InstID=%s, Price=%g, Volume=%ld, Direction=%d, OpenCloseType=%d, %s\n",
		trade.TradeID.c_str(), trade.OrderID.c_str(),
		trade.InstrumentID.c_str(), trade.Price, trade.Volume,
		trade.Direction, trade.OpenCloseType,
		trade.TradeTime.c_str());
}

static void  PrintInstrumentInfo(const XFinApi::TradeApi::Instrument &inst)
{
	printf(" ExchangeID=%s, ProductID=%s, ID=%s, Name=%s\n",
		inst.ExchangeID.c_str(), inst.ProductID.c_str(),
		inst.InstrumentID.c_str(), inst.InstrumentName.c_str());
}

static void  PrintPositionInfo(const XFinApi::TradeApi::Position &pos)
{
	printf("  InstID=%s, PositionYesDirection=%d, PosYesterday=%ld, BuyPosition=%ld, SellPosition=%ld, NetPosition=%ld\n",
		pos.InstrumentID.c_str(), pos.PositionYesDirection, DEFAULT_FILTER(pos.PositionYesterday),
		DEFAULT_FILTER(pos.BuyPosition), DEFAULT_FILTER(pos.SellPosition),
		DEFAULT_FILTER(pos.NetPosition));
}

static void  PrintAccountInfo(const XFinApi::TradeApi::Account &acc)
{
	printf("  Available=%g, NetAssetValue=%g, CommodityPL=%g, IMargin=%g, MMargin=%g, CreditLimit=%g, CashBal=%g\n",
		DEFAULT_FILTER(acc.Available), DEFAULT_FILTER(acc.NetAssetValue), DEFAULT_FILTER(acc.CommodityPL), DEFAULT_FILTER(acc.IMargin),
		DEFAULT_FILTER(acc.MMargin), DEFAULT_FILTER(acc.CreditLimit), DEFAULT_FILTER(acc.CashBal));
}

static bool TimeIsSmaller(const std::string &lhs, const std::string &rhs)
{
	int h1, m1, s1, h2, m2, s2;
	sscanf_s(lhs.c_str(), "%d:%d:%d", &h1, &m1, &s1);
	sscanf_s(rhs.c_str(), "%d:%d:%d", &h2, &m2, &s2);

	if (h1 == h2)
	{
		if (m1 == m2)
			return s1 < s2;

		return m1 < m2;
	}
	return h1 < h2;
}

//////////////////////////////////////////////////////////////////////////////////
//API 创建失败错误码的含义，其他错误码的含义参见TradeApi_win32x86\Cpp\ApiEnum.h文件
static const char *StrCreateErrors[] = {
	"无错误",
	"头文件与接口版本不匹配",
	"头文件与实现版本不匹配",
	"实现加载失败",
	"实现入口未找到",
	"创建实例失败",
	"无授权文件",
	"授权版本不符",
	"最后一次通信超限",
	"机器码错误",
	"认证文件到期",
	"认证超时"
};

//////////////////////////////////////////////////////////////////////////////////
//行情事件
class MarketEvent : public XFinApi::TradeApi::MarketListener
{
public:
	MarketEvent() {}
	~MarketEvent() {}

	void OnNotify(const XFinApi::TradeApi::NotifyParams & notifyParams) override
	{
		printf("* Market");
		PrintNotifyInfo(notifyParams);

		//连接成功后可订阅合约
		if ((int)XFinApi::TradeApi::Action::Open == notifyParams.Action &&
			(int)XFinApi::TradeApi::Result::Success == notifyParams.Result && market)
		{
			//订阅
			XFinApi::TradeApi::QueryParams param;
			param.InstrumentID = Cfg.InstrumentID;
			market->Subscribe(param);
		}

		//ToDo ...
	}

	void OnSubscribed(const XFinApi::TradeApi::QueryParams &instInfo) override
	{
		PrintSubscribedInfo(instInfo);

		//ToDo ...
	}

	void OnUnsubscribed(const XFinApi::TradeApi::QueryParams &instInfo) override
	{
		PrintUnsubscribedInfo(instInfo);

		//ToDo ...
	}

	void OnTick(const XFinApi::TradeApi::Tick &tick) override
	{
		if (Cfg.SellPrice1 <= 0 && Cfg.BuyPrice1 <= 0)
			PrintTickInfo(tick);

		Cfg.SellPrice1 = tick.AskPrice[0];
		Cfg.BuyPrice1 = tick.BidPrice[0];

		//ToDo ...
	}
};

//////////////////////////////////////////////////////////////////////////////////
//交易事件
class TradeEvent : public XFinApi::TradeApi::TradeListener
{
public:
	TradeEvent() {}
	~TradeEvent() {}

	void OnNotify(const XFinApi::TradeApi::NotifyParams &notifyParams) override
	{
		printf("* Trade");
		PrintNotifyInfo(notifyParams);

		//ToDo ...
	}

	void OnUpdateOrder(const XFinApi::TradeApi::Order &order) override
	{
		printf("- OnUpdateOrder:\n");
		PrintOrderInfo(order);

		//ToDo ...
	}

	void OnUpdateTradeOrder(const XFinApi::TradeApi::TradeOrder &trade) override
	{
		printf("- OnUpdateTradeOrder:\n");
		PrintTradeInfo(trade);

		//ToDo ...
	}

	void OnQueryOrder(const std::vector<XFinApi::TradeApi::Order> &orders) override
	{
		printf("- OnQueryOrder:\n");

		std::vector<XFinApi::TradeApi::Order> sortedOrders = orders;
		std::sort(sortedOrders.begin(), sortedOrders.end(), [this](const XFinApi::TradeApi::Order &lhs, const XFinApi::TradeApi::Order &rhs)
		{
			return TimeIsSmaller(lhs.OrderTime, rhs.OrderTime);
		});

		for (const XFinApi::TradeApi::Order &order : sortedOrders)
		{
			PrintOrderInfo(order);

			//ToDo ...
		}
	}

	void OnQueryTradeOrder(const std::vector<XFinApi::TradeApi::TradeOrder> &trades) override
	{
		printf("- OnQueryTradeOrder:\n");

		std::vector<XFinApi::TradeApi::TradeOrder> sortedTradeOrders = trades;
		std::sort(sortedTradeOrders.begin(), sortedTradeOrders.end(), [this](const XFinApi::TradeApi::TradeOrder &lhs, const XFinApi::TradeApi::TradeOrder &rhs)
		{
			return TimeIsSmaller(lhs.TradeTime, rhs.TradeTime);
		});

		for (const XFinApi::TradeApi::TradeOrder &trade : sortedTradeOrders)
		{
			PrintTradeInfo(trade);

			//ToDo ...
		}
	}

	void OnQueryInstrument(const std::vector<XFinApi::TradeApi::Instrument> &insts) override
	{
		printf("- OnQueryInstrument:\n");

		for (const XFinApi::TradeApi::Instrument &inst : insts)
		{
			PrintInstrumentInfo(inst);

			//ToDo ...
		}
	}

	void OnQueryPosition(const std::vector<XFinApi::TradeApi::Position> &posInfos) override
	{
		printf("- OnQueryPosition\n");
		for (const XFinApi::TradeApi::Position &pos : posInfos)
			PrintPositionInfo(pos);

		//ToDo ...
	}

	void OnQueryAccount(const XFinApi::TradeApi::Account &accInfo) override
	{
		printf("- OnQueryAccount\n");
		PrintAccountInfo(accInfo);

		//ToDo ...
	}
};

//////////////////////////////////////////////////////////////////////////////////
//行情测试
void MarketTest()
{
	//创建 IMarket
	//const char* path 指 xxx.exe 同级子目录中的 xxx.dll 文件
	int err = -1;
#ifdef _DEBUG
	market = XFinApi_CreateMarketApi("TradeApi_win32x86/Api/SPTradeApi_R8.75.4/XFinApi.SPTradeApid.dll", &err);
#else
	market = XFinApi_CreateMarketApi("TradeApi_win32x86/Api/SPTradeApi_R8.75.4/XFinApi.SPTradeApi.dll", &err);
#endif

	if (err || !market)
	{
		printf("* Market XFinApiCreateError=%s;\n", StrCreateErrors[err]);
		return;
	}

	//注册事件
	marketEvent = new MarketEvent();
	market->SetListener(marketEvent);

	//连接服务器
	XFinApi::TradeApi::OpenParams openParams;
	openParams.HostAddress = Cfg.HostAddress;
	openParams.UserID = Cfg.UserName;
	openParams.Password = Cfg.Password;
	openParams.Configs["AppID"] = Cfg.AppID;
	openParams.Configs["License"] = Cfg.License;
	market->Open(openParams);

	/*
	连接成功后才能执行订阅行情等操作，检测方法有两种：
	1、IMarket::IsOpened()=true
	2、MarketListener::OnNotify中
	(int)XFinApi::TradeApi::Action::Open == notifyParams.Action &&
	(int)XFinApi::TradeApi::Result::Success == notifyParams.Result
	*/

	/* 行情相关方法
	while (!market->IsOpened())
		std::this_thread::sleep_for(std::chrono::seconds(1));

	//订阅行情，已在MarketEvent::OnNotify中订阅
	XFinApi::TradeApi::QueryParams param;
	param.InstrumentID = Cfg.InstrumentID;
	market->Subscribe(param);

	//取消订阅行情
	market->Unsubscribe(param);
	*/
}

//////////////////////////////////////////////////////////////////////////////////
//交易测试
void TradeTest()
{
	//创建 ITrade
	//const char* path 指 xxx.exe 同级子目录中的 xxx.dll 文件
	int err = -1;
#ifdef _DEBUG
	trade = XFinApi_CreateTradeApi("TradeApi_win32x86/Api/SPTradeApi_R8.75.4/XFinApi.SPTradeApid.dll", &err);
#else
	trade = XFinApi_CreateTradeApi("TradeApi_win32x86/Api/SPTradeApi_R8.75.4/XFinApi.SPTradeApi.dll", &err);
#endif
	if (err && !trade)
	{
		printf("* Trade XFinApiCreateError=%s;\n", StrCreateErrors[err]);
		return;
	}

	//注册事件
	tradeEvent = new TradeEvent;
	trade->SetListener(tradeEvent);

	//连接服务器
	XFinApi::TradeApi::OpenParams openParams;
	openParams.HostAddress = Cfg.HostAddress;
	openParams.UserID = Cfg.UserName;
	openParams.Password = Cfg.Password;
	openParams.Configs["AppID"] = Cfg.AppID;
	openParams.Configs["License"] = Cfg.License;
	trade->Open(openParams);

	/*
	//连接成功后才能执行查询、委托等操作，检测方法有两种：
	1、ITrade::IsOpened()=true
	2、TradeListener::OnNotify中
	(int)XFinApi::TradeApi::Action::Open == notifyParams.Action &&
	(int)XFinApi::TradeApi::Result::Success == notifyParams.Result
	*/
	while (!trade->IsOpened())
		std::this_thread::sleep_for(std::chrono::seconds(1));

	XFinApi::TradeApi::QueryParams qryParam;
	qryParam.InstrumentID = Cfg.InstrumentID;

	//查询委托单
	std::this_thread::sleep_for(std::chrono::seconds(1));//有些接口查询有间隔限制
	std::cout << "Press any key to QueryOrder.\n";
	getchar();
	trade->QueryOrder(qryParam);

	//查询成交单
	std::this_thread::sleep_for(std::chrono::seconds(1));
	std::cout << "Press any key to QueryTradeOrder.\n";
	getchar();
	trade->QueryTradeOrder(qryParam);

	//查询合约
	std::this_thread::sleep_for(std::chrono::seconds(1));
	qryParam.ProductID = Cfg.ProductID;
	std::cout << "Press any key to QueryInstrument.\n";
	getchar();
	trade->QueryInstrument(qryParam);

	//查询持仓
	std::this_thread::sleep_for(std::chrono::seconds(1));
	std::cout << "Press any key to QueryPosition.\n";
	getchar();
	trade->QueryPosition(qryParam);

	//查询账户
	std::this_thread::sleep_for(std::chrono::seconds(1));
	std::cout << "Press any key to QueryAccount.\n";
	getchar();
	trade->QueryAccount(qryParam);

	//委托下单
	std::this_thread::sleep_for(std::chrono::seconds(1));
	std::cout << "Press any key to OrderAction.\n";
	getchar();
	XFinApi::TradeApi::Order order;
	order.InstrumentID = Cfg.InstrumentID;
	order.Price = Cfg.SellPrice1;
	order.PricePrecision = 3;
	order.Volume = 1;
	order.Direction = XFinApi::TradeApi::TradeDirection::Buy;
	order.OpenCloseType = XFinApi::TradeApi::OpenCloseKind::Open;

	//下单高级选项，可选择性设置
	order.ActionType = XFinApi::TradeApi::OrderActionKind::Insert;//下单
	order.OrderType = XFinApi::TradeApi::OrderKind::Order;//标准单
	order.PriceCond = XFinApi::TradeApi::PriceCondition::LimitPrice;//限价
	order.VolumeCond = XFinApi::TradeApi::VolumeCondition::AnyVolume;//任意数量
	order.TimeCond = XFinApi::TradeApi::TimeCondition::GFD;//当日有效
	order.ContingentCond = XFinApi::TradeApi::ContingentCondKind::Immediately;//立即
	order.HedgeType = XFinApi::TradeApi::HedgeKind::Speculation;//投机
	order.ExecResult = XFinApi::TradeApi::ExecResultKind::NoExec;//没有执行

	trade->OrderAction(order);
}

int main()
{
	//可在Config类中修改用户名、密码、合约等信息

	MarketTest();
	TradeTest();

	std::this_thread::sleep_for(std::chrono::seconds(2));
	std::cout << "Press any key to close.\n";
	getchar();

	//关闭连接
	if (market)
	{
		market->Close();
		XFinApi_ReleaseMarketApi(market);//必须释放资源
	}
	if (trade)
	{
		trade->Close();
		XFinApi_ReleaseTradeApi(trade);//必须释放资源
	}
	//清理事件
	if (marketEvent)
	{
		delete marketEvent;
		marketEvent = nullptr;
	}
	if (tradeEvent)
	{
		delete tradeEvent;
		tradeEvent = nullptr;
	}

	std::this_thread::sleep_for(std::chrono::seconds(1));
	std::cout << "Closed.\n";
	getchar();

	return 0;
}
