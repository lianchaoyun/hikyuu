/*
 * _TradeCose.cpp
 *
 *  Created on: 2013-2-14
 *      Author: fasiondog
 */

#include <boost/python.hpp>
#include <hikyuu/trade_manage/TradeCostBase.h>
#include "../_Parameter.h"
#include "../pickle_support.h"

using namespace boost::python;
using namespace hku;

class TradeCostWrap : public TradeCostBase, public wrapper<TradeCostBase> {
public:
    TradeCostWrap(const string& name) : TradeCostBase(name) {}

    CostRecord getBuyCost(const Datetime& datetime, const Stock& stock, price_t price,
                          double num) const {
        return this->get_override("getBuyCost")(datetime, stock, price, num);
    }

    CostRecord getSellCost(const Datetime& datetime, const Stock& stock, price_t price,
                           double num) const {
        return this->get_override("getSellCost")(datetime, stock, price, num);
    }

    TradeCostPtr _clone() {
        return this->get_override("_clone")();
    }

    CostRecord getBorrowCashCost(const Datetime& datetime, price_t cash) const {
        if (override getBorrowCashCost = get_override("getBorrowCashCost")) {
            return getBorrowCashCost(datetime, cash);
        }
        return TradeCostBase::getBorrowCashCost(datetime, cash);
    }

    CostRecord default_getBorrowCashCost(const Datetime& datetime, price_t cash) const {
        return this->TradeCostBase::getBorrowCashCost(datetime, cash);
    }

    CostRecord getReturnCashCost(const Datetime& borrow_datetime, const Datetime& return_datetime,
                                 price_t cash) const {
        if (override getReturnCashCost = get_override("getReturnCashCost")) {
            return getReturnCashCost(borrow_datetime, return_datetime, cash);
        }
        return TradeCostBase::getReturnCashCost(borrow_datetime, return_datetime, cash);
    }

    CostRecord default_getReturnCashCost(const Datetime& borrow_datetime,
                                         const Datetime& return_datetime, price_t cash) const {
        return this->TradeCostBase::getReturnCashCost(borrow_datetime, return_datetime, cash);
    }

    CostRecord getBorrowStockCost(const Datetime& datetime, const Stock& stock, price_t price,
                                  double num) const {
        if (override getBorrowStockCost = get_override("getBorrowStockCost")) {
            return getBorrowStockCost(datetime, stock, price, num);
        }
        return TradeCostBase::getBorrowStockCost(datetime, stock, price, num);
    }

    CostRecord default_getBorrowStockCost(const Datetime& datetime, const Stock& stock,
                                          price_t price, double num) const {
        return this->TradeCostBase::getBorrowStockCost(datetime, stock, price, num);
    }

    CostRecord getReturnStockCost(const Datetime& borrow_datetime, const Datetime& return_datetime,
                                  const Stock& stock, price_t price, double num) const {
        if (override getReturnStockCost = get_override("getReturnStockCost")) {
            return getReturnStockCost(borrow_datetime, return_datetime, stock, price, num);
        }
        return TradeCostBase::getReturnStockCost(borrow_datetime, return_datetime, stock, price,
                                                 num);
    }

    CostRecord default_getReturnStockCost(const Datetime& borrow_datetime,
                                          const Datetime& return_datetime, const Stock& stock,
                                          price_t price, double num) const {
        return this->TradeCostBase::getReturnStockCost(borrow_datetime, return_datetime, stock,
                                                       price, num);
    }
};

void export_TradeCost() {
    class_<TradeCostWrap, boost::noncopyable>("TradeCostBase", R"(????????????????????????

    ????????????????????????????????????

    :py:meth:`TradeCostBase.getBuyCost` - ??????????????????????????????
    :py:meth:`TradeCostBase.getSellCost` - ??????????????????????????????
    :py:meth:`TradeCostBase._clone` - ??????????????????????????????)",

                                              init<const string&>())

      .def(self_ns::str(self))
      .def(self_ns::repr(self))

      .add_property(
        "name", make_function(&TradeCostBase::name, return_value_policy<copy_const_reference>()),
        "??????????????????")

      .def("get_param", &TradeCostBase::getParam<boost::any>, R"(get_param(self, name)

    ?????????????????????

    :param str name: ????????????
    :return: ?????????
    :raises out_of_range: ????????????)")

      .def("set_param", &TradeCostBase::setParam<object>, R"(set_param(self, name, value)

    ????????????

    :param str name: ????????????
    :param value: ?????????
    :raises logic_error: Unsupported type! ????????????????????????)")

      .def("clone", &TradeCostBase::clone, "????????????")

      .def("get_buy_cost", pure_virtual(&TradeCostBase::getBuyCost),
           R"(get_buy_cost(self, datetime, stock, price, num)
    
        ????????????????????????????????????
        
        :param Datetime datetime: ????????????
        :param Stock stock: ????????????
        :param float price: ????????????
        :param int num: ????????????
        :return: ??????????????????
        :rtype: CostRecord)")

      .def("get_sell_cost", pure_virtual(&TradeCostBase::getSellCost),
           R"(get_sell_cost(self, datetime, stock, price, num)
    
        ????????????????????????????????????
        
        :param Datetime datetime: ????????????
        :param Stock stock: ????????????
        :param float price: ????????????
        :param int num: ????????????
        :return: ??????????????????
        :rtype: CostRecord)")

      //.def("getBorrowCashCost", &TradeCostBase::getBorrowCashCost,
      //     &TradeCostWrap::default_getBorrowCashCost)

      //.def("getReturnCashCost", &TradeCostBase::getReturnCashCost,
      //     &TradeCostWrap::default_getReturnCashCost)

      //.def("getBorrowStockCost", &TradeCostBase::getBorrowStockCost,
      //&TradeCostWrap::default_getBorrowStockCost) .def("getReturnStockCost",
      //&TradeCostBase::getReturnStockCost, &TradeCostWrap::default_getReturnStockCost)

      .def("_clone", pure_virtual(&TradeCostBase::_clone), "????????????????????????????????????")

#if HKU_PYTHON_SUPPORT_PICKLE
      .def_pickle(name_init_pickle_suite<TradeCostBase>())
#endif
      ;
    register_ptr_to_python<TradeCostPtr>();
}
