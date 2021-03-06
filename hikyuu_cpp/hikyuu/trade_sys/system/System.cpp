/*
 * SystemBase.cpp
 *
 *  Created on: 2013-3-3
 *      Author: fasiondog
 */

#include "System.h"

namespace hku {

HKU_API std::ostream& operator<<(std::ostream& os, const System& sys) {
    string strip(",\n");
    string space("  ");
    os << "System{\n"
       << space << sys.name() << strip << space << sys.getTO().getQuery() << strip << space
       << sys.getStock() << strip << space << sys.getParameter() << strip << space << sys.getEV()
       << strip << space << sys.getCN() << strip << space << sys.getMM() << strip << space
       << sys.getSG() << strip << space << sys.getST() << strip << space << sys.getTP() << strip
       << space << sys.getPG() << strip << space << sys.getSP() << strip << space
       << (sys.getTM() ? sys.getTM()->str() : "TradeManager(NULL)") << strip << "}";
    return os;
}

HKU_API std::ostream& operator<<(std::ostream& os, const SystemPtr& sys) {
    if (sys) {
        os << *sys;
    } else {
        os << "System(NULL)";
    }
    return os;
}

System::System()
: m_name("SYS_Simple"),
  m_pre_ev_valid(true),  // must true
  m_pre_cn_valid(true),  // must true
  m_buy_days(0),
  m_sell_short_days(0),
  m_lastTakeProfit(0.0),
  m_lastShortTakeProfit(0.0) {
    initParam();
}

System::System(const string& name)
: m_name(name),
  m_pre_ev_valid(true),
  m_pre_cn_valid(true),
  m_buy_days(0),
  m_sell_short_days(0),
  m_lastTakeProfit(0.0),
  m_lastShortTakeProfit(0.0) {
    initParam();
}

System::System(const TradeManagerPtr& tm, const MoneyManagerPtr& mm, const EnvironmentPtr& ev,
               const ConditionPtr& cn, const SignalPtr& sg, const StoplossPtr& st,
               const StoplossPtr& tp, const ProfitGoalPtr& pg, const SlippagePtr& sp,
               const string& name)
: m_tm(tm),
  m_mm(mm),
  m_ev(ev),
  m_cn(cn),
  m_sg(sg),
  m_st(st),
  m_tp(tp),
  m_pg(pg),
  m_sp(sp),
  m_name(name),
  m_pre_ev_valid(true),
  m_pre_cn_valid(true),
  m_buy_days(0),
  m_sell_short_days(0),
  m_lastTakeProfit(0.0),
  m_lastShortTakeProfit(0.0) {
    initParam();
}

System::~System() {}

void System::initParam() {
    //?????????????????????????????????????????????????????????0???0?????????????????????1???
    setParam<int>("max_delay_count", 3);

    //????????????????????????bar?????????????????????
    setParam<bool>("delay", true);  //????????????????????????Bar????????????????????????????????????BAR?????????

    //????????????????????????????????????????????????????????????????????????/?????????/??????????????????????????????????????????
    setParam<bool>("delay_use_current_price", true);
    setParam<bool>("tp_monotonic", true);  //??????????????????
    setParam<int>("tp_delay_n", 3);        //????????????????????????
    setParam<bool>("ignore_sell_sg", false);  //????????????????????????????????????/???????????????????????????

    //???????????????????????????????????????????????????
    setParam<bool>("can_trade_when_high_eq_low", false);

    //????????????????????????????????????????????????
    setParam<bool>("ev_open_position", false);

    //???????????????????????????????????????????????????
    setParam<bool>("cn_open_position", false);

    //??????????????????????????????????????????????????????
    setParam<bool>("support_borrow_cash", false);

    //??????????????????????????????????????????????????????
    setParam<bool>("support_borrow_stock", false);
}

void System::reset(bool with_tm, bool with_ev) {
    if (with_tm && m_tm)
        m_tm->reset();
    if (with_ev && m_ev)
        m_ev->reset();
    if (m_cn)
        m_cn->reset();
    if (m_mm)
        m_mm->reset();
    if (m_sg)
        m_sg->reset();
    if (m_st)
        m_st->reset();
    if (m_tp)
        m_tp->reset();
    if (m_pg)
        m_pg->reset();
    if (m_sp)
        m_sp->reset();

    m_kdata = KData();

    //????????????m_stock?????????Portfolio?????????????????????????????????sys?????????stock????????????????????????,
    //??????sys????????????stock?????????????????????????????????????????????reset
    // m_stock

    m_pre_ev_valid = false;  // true;
    m_pre_cn_valid = false;  // true;

    m_buy_days = 0;
    m_sell_short_days = 0;
    m_trade_list.clear();
    m_lastTakeProfit = 0.0;
    m_lastShortTakeProfit = 0.0;

    m_buyRequest.clear();
    m_sellRequest.clear();
    m_sellShortRequest.clear();
    m_buyShortRequest.clear();
}

void System::setTO(const KData& kdata) {
    m_kdata = kdata;
    m_stock = kdata.getStock();

    // sg->setTO?????????cn->setTO?????????cn????????????sg?????????sg???????????????
    if (m_sg)
        m_sg->setTO(kdata);
    if (m_cn)
        m_cn->setTO(kdata);
    if (m_st)
        m_st->setTO(kdata);
    if (m_tp)
        m_tp->setTO(kdata);
    if (m_pg)
        m_pg->setTO(kdata);
    if (m_sp)
        m_sp->setTO(kdata);

    KQuery query = kdata.getQuery();
    if (m_ev)
        m_ev->setQuery(query);
    if (m_mm)
        m_mm->setQuery(query);
}

SystemPtr System::clone() {
    SystemPtr p = make_shared<System>();
    if (m_tm)
        p->m_tm = m_tm->clone();
    if (m_ev)
        p->m_ev = m_ev->clone();
    if (m_mm)
        p->m_mm = m_mm->clone();
    if (m_cn)
        p->m_cn = m_cn->clone();
    if (m_sg)
        p->m_sg = m_sg->clone();
    if (m_st)
        p->m_st = m_st->clone();
    if (m_tp)
        p->m_tp = m_tp->clone();
    if (m_pg)
        p->m_pg = m_pg->clone();
    if (m_sp)
        p->m_sp = m_sp->clone();

    p->m_params = m_params;
    p->m_name = m_name;
    p->m_stock = m_stock;
    p->m_kdata = m_kdata;

    p->m_pre_ev_valid = m_pre_ev_valid;
    p->m_pre_cn_valid = m_pre_cn_valid;

    p->m_buy_days = m_buy_days;
    p->m_sell_short_days = m_sell_short_days;
    p->m_trade_list = m_trade_list;
    p->m_lastTakeProfit = m_lastTakeProfit;
    p->m_lastShortTakeProfit = m_lastShortTakeProfit;

    p->m_buyRequest = m_buyRequest;
    p->m_sellRequest = m_sellRequest;
    p->m_sellShortRequest = m_sellShortRequest;
    p->m_buyShortRequest = m_buyShortRequest;

    return p;
}

void System::_buyNotifyAll(const TradeRecord& record) {
    if (m_mm)
        m_mm->buyNotify(record);
    if (m_pg)
        m_pg->buyNotify(record);
}

void System::_sellNotifyAll(const TradeRecord& record) {
    if (m_mm)
        m_mm->sellNotify(record);
    if (m_pg)
        m_pg->sellNotify(record);
}

bool System::readyForRun() {
    HKU_ERROR_IF_RETURN(!m_tm, false, "Not setTradeManager!");
    HKU_ERROR_IF_RETURN(!m_mm, false, "Not setMoneyManager!");
    HKU_ERROR_IF_RETURN(!m_sg, false, "Not setSignal!");

    //?????????????????????????????????????????????????????????????????????????????????????????????false
    //?????????????????????????????????????????????????????????????????????
    if (m_ev)
        m_pre_ev_valid = false;

    if (m_cn) {
        m_cn->setTM(m_tm);
        m_cn->setSG(m_sg);
        m_pre_cn_valid = false;  //??????????????????????????????????????????false
    }

    if (m_mm)
        m_mm->setTM(m_tm);
    if (m_pg)
        m_pg->setTM(m_tm);
    if (m_st)
        m_st->setTM(m_tm);
    if (m_tp)
        m_tp->setTM(m_tm);

    m_tm->setParam<bool>("support_borrow_cash", getParam<bool>("support_borrow_cash"));
    m_tm->setParam<bool>("support_borrow_stock", getParam<bool>("support_borrow_stock"));

    return true;
}

void System::run(const KQuery& query, bool reset) {
    HKU_ERROR_IF_RETURN(m_stock.isNull(), void(), "m_stock is NULL!");

    // reset?????????readyForRun???????????????m_pre_cn_valid???m_pre_ev_valid??????????????????????????????
    if (reset)
        this->reset(true, true);

    HKU_IF_RETURN(!readyForRun(), void());

    // m_stock = stock; ???setTO?????????
    KData kdata = m_stock.getKData(query);
    HKU_IF_RETURN(kdata.empty(), void());

    setTO(kdata);
    size_t total = kdata.size();
    for (size_t i = 0; i < total; ++i) {
        if (kdata[i].datetime >= m_tm->initDatetime()) {
            runMoment(kdata[i]);
        }
    }
}

void System::run(const Stock& stock, const KQuery& query, bool reset) {
    m_stock = stock;
    run(query, reset);
}

void System::clearDelayRequest() {
    m_buyRequest.clear();
    m_sellRequest.clear();
    m_sellShortRequest.clear();
    m_buyShortRequest.clear();
}

TradeRecord System::runMoment(const KRecord& record) {
    m_buy_days++;
    m_sell_short_days++;
    return _runMoment(record);
}

TradeRecord System::runMoment(const Datetime& datetime) {
    KRecord today = m_kdata.getKRecord(datetime);
    if (today.isValid()) {
        m_buy_days++;
        m_sell_short_days++;
        return _runMoment(today);
    }
    return TradeRecord();
}

TradeRecord System::_runMoment(const KRecord& today) {
    TradeRecord result;
    if ((today.highPrice == today.lowPrice || today.closePrice > today.highPrice ||
         today.closePrice < today.lowPrice) &&
        !getParam<bool>("can_trade_when_high_eq_low")) {
        return result;
    }

    //?????????????????????????????????
    result = _processRequest(today);

    //----------------------------------------------------------
    // ????????????????????????
    //----------------------------------------------------------

    bool current_ev_valid = _environmentIsValid(today.datetime);

    //????????????????????????
    if (!current_ev_valid) {
        TradeRecord tr;
        //????????????????????????????????????????????????
        if (m_tm->have(m_stock)) {
            tr = _sell(today, PART_ENVIRONMENT);
        }

        m_pre_ev_valid = current_ev_valid;
        return tr.isNull() ? result : tr;
    }

    //?????????????????????????????????
    if (!m_pre_ev_valid) {
        //????????????????????????????????????????????????
        if (getParam<bool>("ev_open_position")) {
            TradeRecord tr = _buy(today, PART_ENVIRONMENT);
            m_pre_ev_valid = current_ev_valid;
            return tr.isNull() ? result : tr;
        }
    }

    m_pre_ev_valid = current_ev_valid;

    //----------------------------------------------------------
    // ????????????????????????????????????
    //----------------------------------------------------------

    bool current_cn_valid = _conditionIsValid(today.datetime);

    //????????????????????????
    if (!current_cn_valid) {
        TradeRecord tr;
        //????????????????????????????????????????????????
        if (m_tm->have(m_stock)) {
            tr = _sell(today, PART_CONDITION);
        }

        m_pre_cn_valid = current_cn_valid;
        return tr.isNull() ? result : tr;
    }

    //?????????????????????????????????
    if (!m_pre_cn_valid) {
        //????????????????????????????????????????????????
        if (getParam<bool>("cn_open_position")) {
            TradeRecord tr = _buy(today, PART_CONDITION);
            m_pre_cn_valid = current_cn_valid;
            return tr.isNull() ? result : tr;
        }
    }

    m_pre_cn_valid = current_cn_valid;

    //----------------------------------------------------------
    // ???????????????????????????
    //----------------------------------------------------------

    //?????????????????????
    if (m_sg->shouldBuy(today.datetime)) {
        TradeRecord tr = _buy(today, PART_SIGNAL);
        // if (m_tm->haveShort(m_stock)) _sellShort(today);
        return tr.isNull() ? result : tr;
    }

    //??????????????????
    if (m_sg->shouldSell(today.datetime)) {
        TradeRecord tr;
        if (m_tm->have(m_stock))
            tr = _sell(today, PART_SIGNAL);
        //_buyShort(today, PART_SIGNAL);
        return tr.isNull() ? result : tr;
    }

    //----------------------------------------------------------
    // ????????????????????????????????????
    //----------------------------------------------------------

    price_t current_price = today.closePrice;

    PositionRecord position = m_tm->getPosition(m_stock);
    if (position.number != 0) {
        TradeRecord tr;
        if (current_price <= position.stoploss) {
            tr = _sell(today, PART_STOPLOSS);
            //} else if (current_price >= position.goalPrice) {
        } else if (current_price >= _getGoalPrice(today.datetime, current_price)) {
            tr = _sell(today, PART_PROFITGOAL);
        } else {
            price_t current_take_profile = _getTakeProfitPrice(today.datetime);
            if (current_take_profile != 0.0) {
                if (current_take_profile < m_lastTakeProfit) {
                    current_take_profile = m_lastTakeProfit;
                } else {
                    m_lastTakeProfit = current_take_profile;
                }
                if (current_price <= current_take_profile) {
                    tr = _sell(today, PART_TAKEPROFIT);
                }
            }
        }

        return tr.isNull() ? result : tr;
    }

    return result;
}

TradeRecord System::_buy(const KRecord& today, Part from) {
    TradeRecord result;
    if (getParam<bool>("delay")) {
        _submitBuyRequest(today, from);
        return result;
    } else {
        return _buyNow(today, from);
    }
}

TradeRecord System::_buyNow(const KRecord& today, Part from) {
    TradeRecord result;

    //?????????????????????????????????
    price_t planPrice = today.closePrice;

    //???????????????
    price_t stoploss = _getStoplossPrice(today.datetime, planPrice);

    //???????????????????????????????????????????????????????????????
    if (planPrice <= stoploss) {
        return result;
    }

    //?????????????????????
    double number = _getBuyNumber(today.datetime, planPrice, planPrice - stoploss, from);
    if (number == 0 || number > m_stock.maxTradeNumber()) {
        return result;
    }

    double min_num = m_stock.minTradeNumber();
    if (min_num > 1) {
        number = number / min_num * min_num;
    }

    price_t realPrice = _getRealBuyPrice(today.datetime, planPrice);
    price_t goalPrice = _getGoalPrice(today.datetime, planPrice);
    TradeRecord record =
      m_tm->buy(today.datetime, m_stock, realPrice, number, stoploss, goalPrice, planPrice, from);
    if (BUSINESS_BUY != record.business) {
        return result;
    }

    m_lastTakeProfit = _getTakeProfitPrice(record.datetime);
    m_trade_list.push_back(record);
    _buyNotifyAll(record);
    return record;
}

TradeRecord System::_buyDelay(const KRecord& today) {
    TradeRecord result;
    if (today.highPrice == today.lowPrice && !getParam<bool>("can_trade_when_high_eq_low")) {
        //??????????????????????????????????????????
        _submitBuyRequest(KRecord(m_buyRequest.datetime), m_buyRequest.from);
        return result;
    }

    //???????????????????????????????????????
    price_t planPrice = today.openPrice;  //???????????????????????????

    //?????????????????????????????????
    price_t stoploss = 0;
    double number = 0;
    price_t goalPrice = 0;
    if (getParam<bool>("delay_use_current_price")) {
        //?????????????????????????????????????????????????????????
        stoploss = _getStoplossPrice(today.datetime, planPrice);
        number = _getBuyNumber(today.datetime, planPrice, planPrice - stoploss, m_buyRequest.from);
        goalPrice = _getGoalPrice(today.datetime, planPrice);

    } else {
        stoploss = m_buyRequest.stoploss;
        number = m_buyRequest.number;
        goalPrice = m_buyRequest.goal;
    }

    //??????????????????????????????????????????????????????????????????????????????0
    if (planPrice <= stoploss || number == 0) {
        m_buyRequest.clear();
        return result;
    }

    double min_num = m_stock.minTradeNumber();
    if (min_num > 1) {
        number = number / min_num * min_num;
    }

    price_t realPrice = _getRealBuyPrice(today.datetime, planPrice);
    TradeRecord record = m_tm->buy(today.datetime, m_stock, realPrice, number, stoploss, goalPrice,
                                   planPrice, m_buyRequest.from);
    if (BUSINESS_BUY != record.business) {
        m_buyRequest.clear();
        return result;
    }

    m_buy_days = 0;
    m_lastTakeProfit = realPrice;
    m_trade_list.push_back(record);
    _buyNotifyAll(record);
    m_buyRequest.clear();
    return record;
}

void System::_submitBuyRequest(const KRecord& today, Part from) {
    if (m_buyRequest.valid) {
        if (m_buyRequest.count > getParam<int>("max_delay_count")) {
            //?????????????????????????????????????????????
            m_buyRequest.clear();
            return;
        }
        m_buyRequest.count++;
        m_buyRequest.datetime = today.datetime;
        m_buyRequest.stoploss = _getStoplossPrice(today.datetime, today.closePrice);
        m_buyRequest.goal = _getGoalPrice(today.datetime, today.closePrice);
        m_buyRequest.number =
          _getBuyNumber(today.datetime, today.closePrice, today.closePrice - m_buyRequest.stoploss,
                        m_buyRequest.from);

    } else {
        m_buyRequest.valid = true;
        m_buyRequest.business = BUSINESS_BUY;
        m_buyRequest.datetime = today.datetime;
        m_buyRequest.stoploss = _getStoplossPrice(today.datetime, today.closePrice);
        m_buyRequest.goal = _getGoalPrice(today.datetime, today.closePrice);
        m_buyRequest.number = _getBuyNumber(today.datetime, today.closePrice,
                                            today.closePrice - m_buyRequest.stoploss, from);
        m_buyRequest.from = from;
        m_buyRequest.count = 1;
    }
}

TradeRecord System::_sellForce(const KRecord& today, double num, Part from) {
    HKU_ASSERT_M(from == PART_ALLOCATEFUNDS || from == PART_PORTFOLIO,
                 "Only Allocator or Portfolis can perform this operation!");
    TradeRecord result;
    if (getParam<bool>("delay")) {
        if (m_sellRequest.valid) {
            if (m_sellRequest.count > getParam<int>("max_delay_count")) {
                //?????????????????????????????????????????????
                m_sellRequest.clear();
                return result;
            }
            m_sellRequest.count++;

        } else {
            m_sellRequest.valid = true;
            m_sellRequest.business = BUSINESS_SELL;
            m_sellRequest.count = 1;
        }

        PositionRecord position = m_tm->getPosition(m_stock);
        m_sellRequest.from = from;
        m_sellRequest.datetime = today.datetime;
        m_sellRequest.stoploss = position.stoploss;
        m_sellRequest.goal = position.goalPrice;
        m_sellRequest.number = num;
        return result;

    } else {
        PositionRecord position = m_tm->getPosition(m_stock);
        price_t realPrice = _getRealSellPrice(today.datetime, today.closePrice);
        TradeRecord record = m_tm->sell(today.datetime, m_stock, realPrice, num, position.stoploss,
                                        position.goalPrice, today.closePrice, from);
        m_trade_list.push_back(record);
        _sellNotifyAll(record);
        return record;
    }
}

TradeRecord System::_sell(const KRecord& today, Part from) {
    TradeRecord result;
    if (getParam<bool>("delay")) {
        _submitSellRequest(today, from);
        return result;
    } else {
        return _sellNow(today, from);
    }
}

TradeRecord System::_sellNow(const KRecord& today, Part from) {
    TradeRecord result;
    price_t planPrice = today.closePrice;
    double number = 0;

    //?????????????????????
    price_t stoploss = _getStoplossPrice(today.datetime, planPrice);

    //????????????????????????????????????????????????????????????????????????????????????
    if (planPrice <= stoploss) {
        number = m_tm->getHoldNumber(today.datetime, m_stock);

        //?????????????????????????????????????????????
    } else {
        number = _getSellNumber(today.datetime, planPrice, planPrice - stoploss, from);
        if (number == 0) {
            return result;
        }
    }

    price_t goalPrice = _getGoalPrice(today.datetime, planPrice);
    price_t realPrice = _getRealSellPrice(today.datetime, planPrice);
    TradeRecord record =
      m_tm->sell(today.datetime, m_stock, realPrice, number, stoploss, goalPrice, planPrice, from);
    if (BUSINESS_SELL != record.business) {
        return result;  //??????????????????
    }

    //????????????????????????????????????????????????0
    if (!m_tm->have(m_stock)) {
        m_lastTakeProfit = 0.0;
    } else {
        m_lastTakeProfit = _getTakeProfitPrice(today.datetime);
    }

    m_trade_list.push_back(record);
    _sellNotifyAll(record);
    return record;
}

TradeRecord System::_sellDelay(const KRecord& today) {
    TradeRecord result;
    if (today.highPrice == today.lowPrice && !getParam<bool>("can_trade_when_high_eq_low")) {
        //???????????????????????????????????????????????????????????????
        _submitSellRequest(KRecord(m_sellRequest.datetime), m_sellRequest.from);
        return result;
    }

    price_t planPrice = today.openPrice;  //???????????????????????????

    //????????????????????????????????????
    price_t stoploss = 0.0;
    double number = 0;
    price_t goalPrice = 0.0;

    Part from = m_sellRequest.from;

    if (getParam<bool>("delay_use_current_price")) {
        stoploss = _getStoplossPrice(today.datetime, planPrice);
        if (planPrice < stoploss) {
            number = m_tm->getHoldNumber(today.datetime, m_stock);
        } else {
            number = _getSellNumber(today.datetime, planPrice, planPrice - stoploss, from);
        }
        goalPrice = _getGoalPrice(today.datetime, planPrice);
    } else {
        stoploss = m_sellRequest.stoploss;
        number = m_sellRequest.number;
        goalPrice = m_sellRequest.goal;
    }

    if (number == 0) {
        m_sellRequest.clear();
        return result;
    }

    price_t realPrice = _getRealSellPrice(today.datetime, planPrice);
    TradeRecord record = m_tm->sell(today.datetime, m_stock, realPrice, number, stoploss, goalPrice,
                                    planPrice, m_sellRequest.from);
    if (BUSINESS_SELL != record.business) {
        m_sellRequest.clear();
        return result;  //??????????????????
    }

    //????????????????????????????????????????????????0
    if (!m_tm->have(m_stock)) {
        m_lastTakeProfit = 0.0;
    }

    m_trade_list.push_back(record);
    _sellNotifyAll(record);
    m_sellRequest.clear();
    return record;
}

void System::_submitSellRequest(const KRecord& today, Part from) {
    if (m_sellRequest.valid) {
        if (m_sellRequest.count > getParam<int>("max_delay_count")) {
            //?????????????????????????????????????????????
            m_sellRequest.clear();
            return;
        }
        m_sellRequest.count++;

    } else {
        m_sellRequest.valid = true;
        m_sellRequest.business = BUSINESS_SELL;
        m_sellRequest.count = 1;
    }

    m_sellRequest.from = from;
    m_sellRequest.datetime = today.datetime;
    m_sellRequest.stoploss = _getStoplossPrice(today.datetime, today.closePrice);
    if (today.closePrice <= m_sellRequest.stoploss) {
        m_sellRequest.number = m_tm->getHoldNumber(today.datetime, m_stock);
    } else {
        m_sellRequest.number = _getSellNumber(today.datetime, today.closePrice,
                                              today.closePrice - m_sellRequest.stoploss, from);
    }

    m_sellRequest.goal = _getGoalPrice(today.datetime, today.closePrice);
}

TradeRecord System::_buyShort(const KRecord& today, Part from) {
    TradeRecord result;
    if (getParam<bool>("support_borrow_stock") == false)
        return result;

    if (getParam<bool>("delay")) {
        _submitBuyShortRequest(today, from);
        return result;
    } else {
        return _buyShortNow(today, from);
    }
}

TradeRecord System::_buyShortNow(const KRecord& today, Part from) {
    TradeRecord result;
    if (today.highPrice == today.lowPrice) {
        //??????????????????????????????????????????
        //_submitBuyRequest(m_buyRequest.datetime);
        return result;
    }

    price_t planPrice = today.closePrice;  //???????????????????????????

    //?????????????????????????????????????????????
    price_t stoploss = _getShortStoplossPrice(m_buyRequest.datetime, planPrice);

    //????????????
    double number = _getBuyShortNumber(today.datetime, planPrice, stoploss - planPrice, from);
    if (number == 0) {
        m_buyShortRequest.clear();
        return result;
    }

    //????????????????????????????????????
    PositionRecord pos = m_tm->getShortPosition(m_stock);
    if (pos.number == 0) {
        m_buyShortRequest.clear();
        return result;
    }

    if (number > pos.number) {
        number = pos.number;
    }

    price_t goalPrice = _getShortGoalPrice(today.datetime, planPrice);
    price_t realPrice = _getRealBuyPrice(today.datetime, planPrice);

    TradeRecord record = m_tm->buyShort(today.datetime, m_stock, realPrice, number, stoploss,
                                        goalPrice, planPrice, PART_SIGNAL);
    if (BUSINESS_BUY_SHORT != record.business) {
        m_buyShortRequest.clear();
        return result;
    }

    m_sell_short_days = 0;
    m_lastTakeProfit = realPrice;  //???????????????????????????
    m_trade_list.push_back(record);
    _buyNotifyAll(record);
    m_buyShortRequest.clear();
    return record;
}

TradeRecord System::_buyShortDelay(const KRecord& today) {
    TradeRecord result;
    if (today.highPrice == today.lowPrice) {
        //??????????????????????????????????????????
        //_submitBuyRequest(m_buyRequest.datetime);
        return result;
    }

    price_t planPrice = today.openPrice;  //???????????????????????????

    price_t stoploss = 0.0;
    double number = 0;
    price_t goalPrice = 0.0;
    if (getParam<bool>("delay_use_current_price")) {
        //?????????????????????????????????????????????
        stoploss = _getShortStoplossPrice(m_buyRequest.datetime, planPrice);
        number =
          _getBuyShortNumber(today.datetime, planPrice, stoploss - planPrice, m_buyRequest.from);
        goalPrice = _getShortGoalPrice(today.datetime, planPrice);

    } else {
        stoploss = m_buyShortRequest.stoploss;
        number = m_buyShortRequest.number;
        goalPrice = m_buyShortRequest.goal;
    }

    if (number == 0) {
        m_buyShortRequest.clear();
        return result;
    }

    //????????????????????????????????????
    PositionRecord pos = m_tm->getShortPosition(m_stock);
    if (pos.number == 0) {
        m_buyShortRequest.clear();
        return result;
    }

    if (number > pos.number) {
        number = pos.number;
    }

    price_t realPrice = _getRealBuyPrice(today.datetime, planPrice);
    TradeRecord record = m_tm->buyShort(today.datetime, m_stock, realPrice, number, stoploss,
                                        goalPrice, planPrice, PART_SIGNAL);
    if (BUSINESS_BUY_SHORT != record.business) {
        m_buyShortRequest.clear();
        return result;
    }

    m_sell_short_days = 0;
    m_lastTakeProfit = realPrice;  //???????????????????????????
    m_trade_list.push_back(record);
    _buyNotifyAll(record);
    m_buyShortRequest.clear();
    return result;
}

void System::_submitBuyShortRequest(const KRecord& today, Part from) {
    if (m_buyShortRequest.valid) {
        if (m_buyShortRequest.count > getParam<int>("max_delay_count")) {
            //?????????????????????????????????????????????
            m_buyRequest.clear();
            return;
        }
        m_buyShortRequest.count++;
        m_buyShortRequest.datetime = today.datetime;
        m_buyShortRequest.stoploss = _getShortStoplossPrice(today.datetime, today.closePrice);
        m_buyShortRequest.goal = _getShortGoalPrice(today.datetime, today.closePrice);
        m_buyShortRequest.number =
          _getBuyShortNumber(today.datetime, today.closePrice,
                             m_buyShortRequest.stoploss - today.closePrice, m_buyShortRequest.from);

    } else {
        m_buyShortRequest.valid = true;
        m_buyShortRequest.business = BUSINESS_BUY;
        m_buyShortRequest.datetime = today.datetime;
        m_buyShortRequest.stoploss = _getShortStoplossPrice(today.datetime, today.closePrice);
        m_buyShortRequest.goal = _getShortGoalPrice(today.datetime, today.closePrice);
        m_buyShortRequest.number = _getBuyShortNumber(
          today.datetime, today.closePrice, m_buyShortRequest.stoploss - today.closePrice, from);
        m_buyShortRequest.from = from;
        m_buyShortRequest.count = 1;
    }
}

TradeRecord System::_sellShort(const KRecord& today, Part from) {
    TradeRecord result;
    if (getParam<bool>("support_borrow_stock") == false)
        return result;

    if (getParam<bool>("delay")) {
        _submitSellShortRequest(today, from);
        return result;
    } else {
        return _sellShortNow(today, from);
    }
}

TradeRecord System::_sellShortNow(const KRecord& today, Part from) {
    TradeRecord result;
    if (today.highPrice == today.lowPrice) {
        //????????????????????????????????????????????????
        _submitSellShortRequest(KRecord(today.datetime), from);
        return result;
    }

    price_t planPrice = today.closePrice;

    //???????????????
    price_t stoploss = _getShortStoplossPrice(today.datetime, planPrice);

    double number = _getSellShortNumber(today.datetime, planPrice, stoploss - planPrice, from);
    if (number == 0) {
        m_sellShortRequest.clear();
        return result;
    }

    price_t goalPrice = _getShortGoalPrice(today.datetime, planPrice);
    price_t realPrice = _getRealSellPrice(today.datetime, planPrice);
    TradeRecord record = m_tm->sell(today.datetime, m_stock, realPrice, number, stoploss, goalPrice,
                                    planPrice, PART_SIGNAL);
    if (BUSINESS_SELL_SHORT != record.business) {
        m_sellShortRequest.clear();
        return result;  //??????????????????
    }

    m_sell_short_days = 0;
    m_lastShortTakeProfit = realPrice;
    m_trade_list.push_back(record);
    _sellNotifyAll(record);
    m_sellShortRequest.clear();
    return record;
}

TradeRecord System::_sellShortDelay(const KRecord& today) {
    TradeRecord result;
    if (today.highPrice == today.lowPrice) {
        //???????????????????????????????????????????????????????????????
        _submitSellShortRequest(KRecord(m_sellShortRequest.datetime), m_sellShortRequest.from);
        return result;
    }

    price_t planPrice = today.openPrice;  //???????????????????????????

    //????????????????????????????????????
    price_t stoploss = 0.0;
    double number = 0;
    price_t goalPrice = 0.0;
    if (getParam<bool>("delay_use_current_price")) {
        stoploss = _getShortStoplossPrice(today.datetime, planPrice);
        number = _getSellShortNumber(today.datetime, planPrice, stoploss - planPrice,
                                     m_sellShortRequest.from);
        goalPrice = _getShortGoalPrice(today.datetime, planPrice);
    } else {
        stoploss = m_sellShortRequest.stoploss;
        number = m_sellShortRequest.number;
        goalPrice = m_sellShortRequest.goal;
    }

    if (number == 0) {
        m_sellShortRequest.clear();
        return result;
    }

    price_t realPrice = _getRealSellPrice(today.datetime, planPrice);

    TradeRecord record = m_tm->sell(today.datetime, m_stock, realPrice, number, stoploss, goalPrice,
                                    planPrice, m_sellShortRequest.from);
    if (BUSINESS_SELL_SHORT != record.business) {
        m_sellShortRequest.clear();
        return result;  //??????????????????
    }

    m_sell_short_days = 0;
    m_lastShortTakeProfit = realPrice;
    m_trade_list.push_back(record);
    _sellNotifyAll(record);
    m_sellShortRequest.clear();
    return record;
}

void System::_submitSellShortRequest(const KRecord& today, Part from) {
    if (m_sellShortRequest.valid) {
        if (m_sellShortRequest.count > getParam<int>("max_delay_count")) {
            //?????????????????????????????????????????????
            m_sellShortRequest.clear();
            return;
        }
        m_sellShortRequest.count++;
        m_sellShortRequest.datetime = today.datetime;
        m_sellShortRequest.stoploss = _getStoplossPrice(today.datetime, today.closePrice);
        m_sellShortRequest.goal = _getGoalPrice(today.datetime, today.closePrice);
        m_sellShortRequest.number =
          _getSellNumber(today.datetime, today.closePrice,
                         today.closePrice - m_sellShortRequest.stoploss, m_sellShortRequest.from);

    } else {
        m_sellShortRequest.valid = true;
        m_sellShortRequest.business = BUSINESS_SELL_SHORT;
        m_sellShortRequest.datetime = today.datetime;
        m_sellShortRequest.stoploss = _getStoplossPrice(today.datetime, today.closePrice);
        m_sellShortRequest.goal = _getGoalPrice(today.datetime, today.closePrice);
        m_sellShortRequest.number =
          _getSellNumber(today.datetime, today.closePrice,
                         today.closePrice - m_sellShortRequest.stoploss, m_sellShortRequest.from);
        m_sellShortRequest.from = from;
        m_sellShortRequest.count = 1;
    }
}

TradeRecord System::_processRequest(const KRecord& today) {
    HKU_IF_RETURN(m_buyRequest.valid, _buyDelay(today));
    HKU_IF_RETURN(m_sellRequest.valid, _sellDelay(today));
    HKU_IF_RETURN(m_sellShortRequest.valid, _sellShortDelay(today));
    HKU_IF_RETURN(m_buyShortRequest.valid, _buyShortDelay(today));
    return TradeRecord();
}

bool System::haveDelayRequest() const {
    return (m_buyRequest.valid || m_sellRequest.valid || m_sellShortRequest.valid ||
            m_buyShortRequest.valid);
}

} /* namespace hku */
