######################################################
# SPTradeApiSimpleDemoPythonPyCharmCommunity2018
# www.xfinapi.com
######################################################
import XFinApi_TradeApi
import time


# Python版本说明：Api使用32位python-3.6版本编译，建议开发时也使用该版本。#


# 配置信息
class Config:
    # 注册SP仿真交易账号，http: // demo.spsystem.info: 8000 / DemoAcc / DemoAcc_API.php?lang = 2
    # 地址
    HostAddress = "demo.spsystem.info:8080";
    # 账户
    UserName = "DEMO201806119A"; # 公用测试账户, 其他人也在使用时会登录失败，报错：-11150005, 已登入。为了测试准确，请注册使用您自己的账户。
    Password = "a123456";
    AppID = "SPDEMO";
    License = "5B20847848D9D";

    # 合约
    InstrumentID = "SIZ8";
    ProductID = "SI";

    # 行情
    AskPrice1 = -1;
    BidPrice1 = -1;


# API创建失败错误码的含义，其他错误码的含义参见XTA_W32\Cpp\ApiEnum.h文件
StrCreateErrors = [
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
    "认证超时"]


class MarketEvent(XFinApi_TradeApi.MarketListener):
    cfg = 0
    market = 0

    def __init__(self,market,cfg):
        XFinApi_TradeApi.MarketListener.__init__(self)
        self.market = market
        self.cfg = cfg

    def OnNotify(self, notifyParams: 'NotifyParams'):
        print("* Market")
        str = ""
        for codeinfo in notifyParams.CodeInfos:
            str += "(Code={};LowerCode={};LowerMessage={})".format(
                codeinfo.Code, codeinfo.LowerCode, codeinfo.LowerMessage)
        print(" OnNotify: Action={}, Result={}{}".format(
            notifyParams.ActionType,
            notifyParams.ResultType,
            str
        ))
        if XFinApi_TradeApi.ActionKind_Open == notifyParams.ActionType and XFinApi_TradeApi.ResultKind_Success == notifyParams.ResultType:
            # 订阅
            param = XFinApi_TradeApi.QueryParams()
            param.InstrumentID = self.cfg.InstrumentID
            self.market.Subscribe(param)

    def OnSubscribed(self,instInfo):
        print("- OnSubscribed:{}".format(instInfo.InstrumentID))

    def OnUnsubscribed(self,instInfo):
        print("- OnUnsubscribed:{}".format(instInfo.InstrumentID))

    def OnTick(self,tick):
        if self.cfg.AskPrice1 <= 0 and self.cfg.BidPrice1 <= 0:
            print(" Tick,{} {}, HighestPrice={}, LowestPrice={}, BidPrice0={}, BidVolume0={}, AskPrice0={}, AskVolume0={}, LastPrice={}, LastVolume={}, TradingDay={}, TradingTime={}".format(
                tick.ExchangeID,
                tick.InstrumentID,
                tick.HighestPrice,
                tick.LowestPrice,
                tick.GetBidPrice(0),
                tick.GetBidVolume(0),
                tick.GetAskPrice(0),
                tick.GetAskVolume(0),
                tick.LastPrice,
                tick.LastVolume,
                tick.TradingDay,
                tick.TradingTime))
        self.cfg.AskPrice1 = tick.GetAskPrice(0)
        self.cfg.BidPrice1 = tick.GetBidPrice(0)


class MarketTest:
    market = 0
    cfg = 0
    marketEvent = 0

    def __init__(self,cfg):
        self.cfg = cfg

    def __del__(self):
        if isinstance(self.marketEvent,MarketEvent):
            del self.marketEvent
        if isinstance(self.market,XFinApi_TradeApi.IMarket):
            XFinApi_TradeApi.XFinApi_ReleaseMarketApi(self.market)

    def test(self):
        self.market = XFinApi_TradeApi.XFinApi_CreateMarketApi("XTA_W32/Api/SPApi_R8.75.4/XFinApi.SPTradeApi.dll")
        if isinstance(self.market,int):
            print("* Market XFinApiCreateError={};".format(StrCreateErrors[self.market]))
            return
        else:
            self.market = self.market[0]
        self.marketEvent = MarketEvent(self.market,self.cfg)
        self.market.SetListener(self.marketEvent)
        openParams = XFinApi_TradeApi.OpenParams()
        openParams.HostAddress = self.cfg.HostAddress;
        openParams.UserID = self.cfg.UserName
        openParams.Password = self.cfg.Password
        openParams.Configs["AppID"] = self.cfg.AppID;
        openParams.Configs["License"] = self.cfg.License;
        openParams.IsUTF8 = True
        self.market.Open(openParams)


class TradeEvent(XFinApi_TradeApi.TradeListener):
    cfg = 0
    trade = 0

    def __init__(self,trade,cfg):
        XFinApi_TradeApi.TradeListener.__init__(self)
        self.trade = trade
        self.cfg = cfg

    def OnNotify(selfs,notifyParams):
        print("* Trade")
        str = ""
        for codeinfo in notifyParams.CodeInfos:
            str += "(Code={};LowerCode={};LowerMessage={})".format(
                codeinfo.Code, codeinfo.LowerCode, codeinfo.LowerMessage)
        print(" OnNotify: Action={}, Result={}{}".format(
            notifyParams.ActionType,
            notifyParams.ResultType,
            str
        ))

    def OnUpdateOrder(self,order):
        print("- OnUpdateOrder:")
        print("ProductType={}, ID={}, InstID={}, Price={}, Volume={}, NoTradedVolume={}, Direction={}, "
              "OpenCloseType={}, PriceCond={}, TimeCond={}, VolumeCond={}, Status={}, Msg={}, {}".format(
              order.ProductType, order.OrderID,
              order.InstrumentID, order.Price, order.Volume, order.NoTradedVolume,
              order.Direction, order.OpenCloseType,order.PriceCond, order.TimeCond, order.VolumeCond,
              order.Status, order.StatusMsg, order.OrderTime))

    def OnUpdateTradeOrder(self,trade):
        print("- OnUpdateTradeOrder:")
        print(" ID={}, OrderID={}, InstID={}, Price={}, Volume={}, Direction={}, OpenCloseType={}, {}".format(
            trade.TradeID, trade.OrderID,trade.InstrumentID, trade.Price, trade.Volume,
            trade.Direction, trade.OpenCloseType,trade.TradeTime))

    def OnQueryOrder(self,orders):
        print("- OnQueryOrder:")
        for order in orders:
            print("ProductType={}, ID={}, InstID={}, Price={}, Volume={}, NoTradedVolume={}, Direction={}, "
                  "OpenCloseType={}, PriceCond={}, TimeCond={}, VolumeCond={}, Status={}, Msg={}, {}"
                  .format(
                  order.ProductType, order.OrderID,
                  order.InstrumentID, order.Price, order.Volume, order.NoTradedVolume,
                  order.Direction, order.OpenCloseType,order.PriceCond, order.TimeCond, order.VolumeCond,
                  order.Status, order.StatusMsg, order.OrderTime))

    def OnQueryTradeOrder(self,trades):
        print("- OnQueryTradeOrder:")
        for trade in trades:
            print(" ID={}, OrderID={}, InstID={}, Price={}, Volume={}, Direction={}, OpenCloseType={}, {}".format(
                trade.TradeID, trade.OrderID,trade.InstrumentID, trade.Price, trade.Volume,
                trade.Direction, trade.OpenCloseType,trade.TradeTime))

    def OnQueryInstrument(self,instruments):
        print("- OnQueryInstrument:")
        for inst in instruments:
            print("ExchangeID={}, ProductID={}, ID={}, Name={}".format(inst.ExchangeID, inst.ProductID,inst.InstrumentID, inst.InstrumentName))

    def OnQueryPosition(self,poss):
        print("- OnQueryPosition")
        for pos in poss:
            buyposition = 0
            sellposition = 0
            positioNet = 0
            posyesterday = 0
            if XFinApi_TradeApi.IsDefaultValue(pos.BuyPosition) == False:
                buyposition = pos.BuyPosition
            if XFinApi_TradeApi.IsDefaultValue(pos.SellPosition) == False:
                sellposition = pos.SellPosition
            if XFinApi_TradeApi.IsDefaultValue(pos.NetPosition) == False:
                positioNet = pos.NetPosition
            if XFinApi_TradeApi.IsDefaultValue(pos.PositionYesterday) == False:
                posyesterday = pos.PositionYesterday
                print(" InstID={}, PositionYesterdayDirection={}, PositionYesterday={}, BuyPosition={}, SellPosition={}, NetPosition={}"
                      .format(pos.InstrumentID, pos.PositionYesDirection, posyesterday, buyposition, sellposition, positioNet))

    def OnQueryAccount(self,accInfo):
         print("- OnQueryAccount")
         print("  Available={}, StaticBalance={}, CloseProfit={}, CurrMargin={}, MaintenanceMargin={}, CreditLimit={}, Balance={}"
               .format(accInfo.Available, accInfo.StaticBalance, accInfo.CloseProfit, accInfo.CurrMargin,
                accInfo.MaintenanceMargin, accInfo.CreditLimit, accInfo.Balance))


class TradeTest:
    cfg = 0
    trade = 0
    tradeEvent = 0

    def __init__(self,cfg):
        self.cfg = cfg

    def __del__(self):
        if isinstance(self.tradeEvent,TradeEvent):
            del self.tradeEvent
        if isinstance(self.trade,XFinApi_TradeApi.ITrade):
            XFinApi_TradeApi.XFinApi_ReleaseTradeApi(self.trade)

    def Test(self):
        # 创建ITrade char * path指xxx.exe同级子目录中的xxx.dll文件
        self.trade = XFinApi_TradeApi.XFinApi_CreateTradeApi(
            "XTA_W32/Api/SPApi_R8.75.4/XFinApi.SPTradeApi.dll")
        if isinstance(self.trade,int):
            print("* Trade XFinApiCreateError={};".format(StrCreateErrors[self.trade]))
            return
        else:
            self.trade = self.trade[0]

        self.tradeEvent = TradeEvent(self.trade,self.cfg)
        self.trade.SetListener(self.tradeEvent)
        openParams = XFinApi_TradeApi.OpenParams()
        openParams.HostAddress = self.cfg.HostAddress;
        openParams.UserID = self.cfg.UserName
        openParams.Password = self.cfg.Password
        openParams.Configs["AppID"] = self.cfg.AppID;
        openParams.Configs["License"] = self.cfg.License;
        openParams.IsUTF8 = True

        self.trade.Open(openParams)

        # 连接成功后才能执行查询、委托等操作，检测方法有两种：
        # 1、self.trade.IsOpened() == 1
        # 2、TradeEvent的OnNotify中
        # XFinApi_TradeApi.ActionKind_Open == notifyParams.ActionType and XFinApi_TradeApi.ResultKind_Success == notifyParams.ResultType
        while self.trade.IsOpened() != 1:
            time.sleep(1)

        qryParam = XFinApi_TradeApi.QueryParams()
        qryParam.InstrumentID = self.cfg.InstrumentID

        # 查询委托单 有些接口查询有间隔限制，如：CTP查询间隔为1秒
        time.sleep(1)
        print("Press any key to QueryOrder.")
        input()
        self.trade.QueryOrder(qryParam)

        # 查询成交单
        time.sleep(3)
        print("Press any key to QueryTradeOrder.")
        input()
        self.trade.QueryTradeOrder(qryParam)

        # 查询合约
        time.sleep(3)
        qryParam.ProductID = self.cfg.ProductID;
        print("Press any key to QueryInstrument.")
        input()
        self.trade.QueryInstrument(qryParam)

        # 查询持仓
        time.sleep(3)
        print("Press any key to QueryPosition.")
        input()
        self.trade.QueryPosition(qryParam)

        # 查询账户
        time.sleep(3)
        print("Press any key to QueryAccount.")
        input()
        self.trade.QueryAccount(qryParam)

        # 委托下单
        time.sleep(1)
        print("Press any key to OrderAction.");
        input()
        order = XFinApi_TradeApi.Order()
        order.InstrumentID = self.cfg.InstrumentID
        order.Price = self.cfg.AskPrice1
        order.PricePrecision = 3;
        order.Volume = 1
        order.Direction = XFinApi_TradeApi.DirectionKind_Buy
        order.OpenCloseType = XFinApi_TradeApi.OpenCloseKind_Open
        # 下单高级选项，可选择性设置
        order.ActionType = XFinApi_TradeApi.OrderActionKind_Insert  # 下单
        order.OrderType = XFinApi_TradeApi.OrderKind_Order  # 标准单
        order.PriceCond = XFinApi_TradeApi.PriceConditionKind_LimitPrice  # 限价
        order.VolumeCond = XFinApi_TradeApi.VolumeConditionKind_AnyVolume  # 任意数量
        order.TimeCond = XFinApi_TradeApi.TimeConditionKind_GFD  # 当日有效
        order.ContingentCond = XFinApi_TradeApi.ContingentCondKind_Immediately  # 立即
        order.HedgeType = XFinApi_TradeApi.HedgeKind_Speculation  # 投机
        self.trade.OrderAction(order)


def main():
    cfg = Config
    mt = MarketTest(cfg)
    mt.test()
    tt = TradeTest(cfg)
    tt.Test()
    time.sleep(1)
    del mt
    del tt

if __name__ == "__main__":
    main()
