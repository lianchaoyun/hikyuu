/*
 *  Copyright(C) 2021 hikyuu.org
 *
 *  Create on: 2021-03-22
 *     Author: fasiondog
 */

#pragma once

#include "../RestHandle.h"

namespace hku {

class LogLevelHandle : public RestHandle {
    REST_HANDLE_IMP(LogLevelHandle)

    virtual void run() override {
        json req = getReqJson();
        REQ_CHECK(req.contains("level"), BadRequestErrorCode::MISS_PARAMETER, "Missing key: level");
        REQ_CHECK(req["level"].is_number_integer(), BadRequestErrorCode::WRONG_PARAMETER_TYPE,
                  "level type must be integer");
        int level = req["level"].get<int>();
        if (!req.contains("logger")) {
            set_logger_level(level);
            setResData(R"({"result": true})");
            return;
        }

        REQ_CHECK(req["logger"].is_string(), BadRequestErrorCode::WRONG_PARAMETER_TYPE,
                  "logger type must be string");
        std::string logger = req["logger"].get<std::string>();
        if (have_logger(logger)) {
            set_logger_level(logger, level);
            setResData(R"({"result": true})");
        } else {
            setResData(fmt::format(
              R"({{"result":false, "errcode": 2000, "errmsg":"not exist logger {}"}})", logger));
        }
    }
};

}  // namespace hku