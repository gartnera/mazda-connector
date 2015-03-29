#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <future>
#include <memory>

#include <dbus/dbus.h>

#include "dbus.hpp"
#include "navigation.hpp"

std::future<std::unique_ptr<location>>
GetPosition(void)
{
    return std::async(std::launch::async,
        []() -> std::unique_ptr<location> {
            DBusMessage *msg = dbus_message_new_method_call("com.jci.lds.data", "/com/jci/lds/data", "com.jci.lds.data", "GetPosition");
            DBusPendingCall *pending = nullptr;

            if (!dbus_connection_send_with_reply(service_bus, msg, &pending, -1)) {
                return nullptr;
            }

            dbus_connection_flush(service_bus);
            dbus_message_unref(msg);

            dbus_pending_call_block(pending);
            msg = dbus_pending_call_steal_reply(pending);
            if (!msg) {
                return nullptr;
            }

            location result;
            if (!dbus_message_get_args(msg, nullptr, DBUS_TYPE_INT32, &result.positionAccuracy,
                                                     DBUS_TYPE_UINT64, &result.time,
                                                     DBUS_TYPE_DOUBLE, &result.latitude,
                                                     DBUS_TYPE_DOUBLE, &result.longitude,
                                                     DBUS_TYPE_INT32, &result.altitude,
                                                     DBUS_TYPE_DOUBLE, &result.heading,
                                                     DBUS_TYPE_DOUBLE, &result.velocity,
                                                     DBUS_TYPE_DOUBLE, &result.horizontalAccuracy,
                                                     DBUS_TYPE_DOUBLE, &result.verticalAccuracy,
                                                     DBUS_TYPE_INVALID)) {
                return nullptr;
            }

            dbus_message_unref(msg);

            if (result.latitude == 0.0 && result.longitude == 0.0) {
                // Presumably the car is not actually in the Atlantic Ocean
                return nullptr;
            }

            // Fix the bearing returned by the car
            result.heading += 180.0;
            if (result.heading >= 360.0) {
                result.heading -= 360.0;
            }

            return std::unique_ptr<location>(new location(result));
        });
}

void GuidanceChanged(
    int32_t manueverIcon,
    int32_t manueverDistance,
    int32_t manueverDistanceUnit,
    int32_t speedLimit,
    int32_t speedLimitUnit,
    int32_t laneIcon1,
    int32_t laneIcon2,
    int32_t laneIcon3,
    int32_t laneIcon4,
    int32_t laneIcon5,
    int32_t laneIcon6,
    int32_t laneIcon7,
    int32_t laneIcon8)
{
    dbus_bool_t result;
    DBusMessage *msg = dbus_message_new_signal("/com/NNG/Api/Server", "com.NNG.Api.Server.Guidance", "GuidanceChanged");
    if (!msg) {
        assert(false && "failed to create message");
    }

    result =
        dbus_message_append_args(
            msg,
            DBUS_TYPE_INT32, &manueverIcon,
            DBUS_TYPE_INT32, &manueverDistance,
            DBUS_TYPE_INT32, &manueverDistanceUnit,
            DBUS_TYPE_INT32, &speedLimit,
            DBUS_TYPE_INT32, &speedLimitUnit,
            DBUS_TYPE_INT32, &laneIcon1,
            DBUS_TYPE_INT32, &laneIcon2,
            DBUS_TYPE_INT32, &laneIcon3,
            DBUS_TYPE_INT32, &laneIcon4,
            DBUS_TYPE_INT32, &laneIcon5,
            DBUS_TYPE_INT32, &laneIcon6,
            DBUS_TYPE_INT32, &laneIcon7,
            DBUS_TYPE_INT32, &laneIcon8,
            DBUS_TYPE_INVALID);

    if (!result) {
        assert(false && "failed to append arguments to message");
        exit(1);
    }

    if (!dbus_connection_send(service_bus, msg, nullptr)) {
        assert(false && "failed to send message");
    }

    dbus_message_unref(msg);
}

std::future<uint8_t>
SetHUDDisplayMsgReq(
    uint32_t manueverIcon,
    uint16_t manueverDistance,
    uint8_t manueverDistanceUnit,
    uint16_t speedLimit,
    uint8_t speedLimitUnit)
{
    return std::async(std::launch::async,
        [=](){
            DBusMessage *msg = dbus_message_new_method_call("com.jci.vbs.navi", "/com/jci/vbs/navi",
                                                            "com.jci.vbs.navi","SetHUDDisplayMsgReq");
            DBusPendingCall *pending = nullptr;

            if (!msg) {
                assert(false && "failed to create message");
            }

            DBusMessageIter iter;
            dbus_message_iter_init_append(msg, &iter);

            DBusMessageIter sub;
            if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_STRUCT, nullptr, &sub)) {
               assert(false && "failed to initialize sub-iterator");
            }

            {
                dbus_bool_t result = TRUE;
                result &= dbus_message_iter_append_basic(&sub, DBUS_TYPE_UINT32, &manueverIcon);
                result &= dbus_message_iter_append_basic(&sub, DBUS_TYPE_UINT16, &manueverDistance);
                result &= dbus_message_iter_append_basic(&sub, DBUS_TYPE_BYTE, &manueverDistanceUnit);
                result &= dbus_message_iter_append_basic(&sub, DBUS_TYPE_UINT16, &speedLimit);
                result &= dbus_message_iter_append_basic(&sub, DBUS_TYPE_BYTE, &speedLimitUnit);

                if (!result) {
                    assert(false && "failed to append arguments to struct");
                }
            }

            if (!dbus_message_iter_close_container(&iter, &sub)) {
                assert(false && "failed to close container");
            }

            if (!dbus_connection_send_with_reply(service_bus, msg, &pending, -1)) {
                assert(false && "failed to send message");
            }

            dbus_connection_flush(service_bus);
            dbus_message_unref(msg);

            dbus_pending_call_block(pending);
            msg = dbus_pending_call_steal_reply(pending);
            if (!msg) {
               assert(false && "received null reply");
            }

            uint8_t result;
            if (!dbus_message_get_args(msg, nullptr, DBUS_TYPE_BYTE, &result,
                                                     DBUS_TYPE_INVALID)) {
                assert(false && "failed to get result");
            }

            dbus_message_unref(msg);

            return result;
        });
}

void NotificationBar_Notify(
    int32_t manueverIcon,
    int32_t manueverDistance,
    int32_t manueverDistanceUnit,
    const char *streetName,
    int32_t priority)
{
    dbus_bool_t result;
    DBusMessage *msg = dbus_message_new_signal("/com/NNG/Api/Server", "com.NNG.Api.Server.NotificationBar", "Notify");
    if (!msg) {
        assert(false && "failed to create message");
    }

    result =
        dbus_message_append_args(
            msg,
            DBUS_TYPE_INT32, &manueverIcon,
            DBUS_TYPE_INT32, &manueverDistance,
            DBUS_TYPE_INT32, &manueverDistanceUnit,
            DBUS_TYPE_STRING, &streetName,
            DBUS_TYPE_INT32, &priority,
            DBUS_TYPE_INVALID);

    if (!result) {
        assert(false && "failed to append arguments to message");
    }

    if (!dbus_connection_send(service_bus, msg, nullptr)) {
        assert(false && "failed to send message");
    }

    dbus_message_unref(msg);
}

void updateHUD(
    int32_t manueverIcon,
    int32_t manueverDistance,
    int32_t manueverDistanceUnit,
    int32_t speedLimit,
    int32_t speedLimitUnit,
    const char *streetName,
    int32_t priority)
{
    GuidanceChanged(manueverIcon, manueverDistance, manueverDistanceUnit, speedLimit, speedLimitUnit, 0, 0, 0, 0, 0, 0, 0, 0);
    NotificationBar_Notify(manueverIcon, manueverDistance, manueverDistanceUnit, streetName, priority);
    SetHUDDisplayMsgReq(manueverIcon, manueverDistance, manueverDistanceUnit, speedLimit, speedLimitUnit);
    dbus_connection_flush(service_bus);
}
