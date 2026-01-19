/****************************************************************************
** Meta object code from reading C++ file 'OrderController.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/models/OrderController.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'OrderController.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_atrad__OrderController_t {
    uint offsetsAndSizes[74];
    char stringdata0[23];
    char stringdata1[19];
    char stringdata2[1];
    char stringdata3[19];
    char stringdata4[18];
    char stringdata5[18];
    char stringdata6[10];
    char stringdata7[5];
    char stringdata8[7];
    char stringdata9[17];
    char stringdata10[10];
    char stringdata11[10];
    char stringdata12[7];
    char stringdata13[10];
    char stringdata14[13];
    char stringdata15[12];
    char stringdata16[17];
    char stringdata17[2];
    char stringdata18[15];
    char stringdata19[7];
    char stringdata20[23];
    char stringdata21[5];
    char stringdata22[4];
    char stringdata23[12];
    char stringdata24[4];
    char stringdata25[6];
    char stringdata26[7];
    char stringdata27[16];
    char stringdata28[20];
    char stringdata29[12];
    char stringdata30[11];
    char stringdata31[10];
    char stringdata32[11];
    char stringdata33[10];
    char stringdata34[11];
    char stringdata35[14];
    char stringdata36[13];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_atrad__OrderController_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_atrad__OrderController_t qt_meta_stringdata_atrad__OrderController = {
    {
        QT_MOC_LITERAL(0, 22),  // "atrad::OrderController"
        QT_MOC_LITERAL(23, 18),  // "orderParamsChanged"
        QT_MOC_LITERAL(42, 0),  // ""
        QT_MOC_LITERAL(43, 18),  // "calculationChanged"
        QT_MOC_LITERAL(62, 17),  // "marketDataChanged"
        QT_MOC_LITERAL(80, 17),  // "connectionChanged"
        QT_MOC_LITERAL(98, 9),  // "orderSent"
        QT_MOC_LITERAL(108, 4),  // "json"
        QT_MOC_LITERAL(113, 6),  // "onTick"
        QT_MOC_LITERAL(120, 16),  // "updateInstrument"
        QT_MOC_LITERAL(137, 9),  // "sendOrder"
        QT_MOC_LITERAL(147, 9),  // "direction"
        QT_MOC_LITERAL(157, 6),  // "offset"
        QT_MOC_LITERAL(164, 9),  // "subscribe"
        QT_MOC_LITERAL(174, 12),  // "instrumentId"
        QT_MOC_LITERAL(187, 11),  // "unsubscribe"
        QT_MOC_LITERAL(199, 16),  // "setPriceOriginal"
        QT_MOC_LITERAL(216, 1),  // "p"
        QT_MOC_LITERAL(218, 14),  // "setManualPrice"
        QT_MOC_LITERAL(233, 6),  // "manual"
        QT_MOC_LITERAL(240, 22),  // "updateConnectionStatus"
        QT_MOC_LITERAL(263, 4),  // "core"
        QT_MOC_LITERAL(268, 3),  // "ctp"
        QT_MOC_LITERAL(272, 11),  // "sendCommand"
        QT_MOC_LITERAL(284, 3),  // "cmd"
        QT_MOC_LITERAL(288, 5),  // "price"
        QT_MOC_LITERAL(294, 6),  // "volume"
        QT_MOC_LITERAL(301, 15),  // "estimatedMargin"
        QT_MOC_LITERAL(317, 19),  // "estimatedCommission"
        QT_MOC_LITERAL(337, 11),  // "isAutoPrice"
        QT_MOC_LITERAL(349, 10),  // "isTestMode"
        QT_MOC_LITERAL(360, 9),  // "bidPrices"
        QT_MOC_LITERAL(370, 10),  // "bidVolumes"
        QT_MOC_LITERAL(381, 9),  // "askPrices"
        QT_MOC_LITERAL(391, 10),  // "askVolumes"
        QT_MOC_LITERAL(402, 13),  // "coreConnected"
        QT_MOC_LITERAL(416, 12)   // "ctpConnected"
    },
    "atrad::OrderController",
    "orderParamsChanged",
    "",
    "calculationChanged",
    "marketDataChanged",
    "connectionChanged",
    "orderSent",
    "json",
    "onTick",
    "updateInstrument",
    "sendOrder",
    "direction",
    "offset",
    "subscribe",
    "instrumentId",
    "unsubscribe",
    "setPriceOriginal",
    "p",
    "setManualPrice",
    "manual",
    "updateConnectionStatus",
    "core",
    "ctp",
    "sendCommand",
    "cmd",
    "price",
    "volume",
    "estimatedMargin",
    "estimatedCommission",
    "isAutoPrice",
    "isTestMode",
    "bidPrices",
    "bidVolumes",
    "askPrices",
    "askVolumes",
    "coreConnected",
    "ctpConnected"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_atrad__OrderController[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      14,   14, // methods
      13,  136, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   98,    2, 0x06,   14 /* Public */,
       3,    0,   99,    2, 0x06,   15 /* Public */,
       4,    0,  100,    2, 0x06,   16 /* Public */,
       5,    0,  101,    2, 0x06,   17 /* Public */,
       6,    1,  102,    2, 0x06,   18 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       8,    1,  105,    2, 0x0a,   20 /* Public */,
       9,    1,  108,    2, 0x0a,   22 /* Public */,
      10,    2,  111,    2, 0x0a,   24 /* Public */,
      13,    1,  116,    2, 0x0a,   27 /* Public */,
      15,    1,  119,    2, 0x0a,   29 /* Public */,
      16,    1,  122,    2, 0x0a,   31 /* Public */,
      18,    1,  125,    2, 0x0a,   33 /* Public */,
      20,    2,  128,    2, 0x0a,   35 /* Public */,
      23,    1,  133,    2, 0x0a,   38 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    7,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   11,   12,
    QMetaType::Void, QMetaType::QString,   14,
    QMetaType::Void, QMetaType::QString,   14,
    QMetaType::Void, QMetaType::Double,   17,
    QMetaType::Void, QMetaType::Bool,   19,
    QMetaType::Void, QMetaType::Bool, QMetaType::Bool,   21,   22,
    QMetaType::Void, QMetaType::QString,   24,

 // properties: name, type, flags
      14, QMetaType::QString, 0x00015103, uint(0), 0,
      25, QMetaType::Double, 0x00015103, uint(0), 0,
      26, QMetaType::Int, 0x00015103, uint(0), 0,
      27, QMetaType::Double, 0x00015001, uint(1), 0,
      28, QMetaType::Double, 0x00015001, uint(1), 0,
      29, QMetaType::Bool, 0x00015003, uint(0), 0,
      30, QMetaType::Bool, 0x00015003, uint(0), 0,
      31, QMetaType::QVariantList, 0x00015001, uint(2), 0,
      32, QMetaType::QVariantList, 0x00015001, uint(2), 0,
      33, QMetaType::QVariantList, 0x00015001, uint(2), 0,
      34, QMetaType::QVariantList, 0x00015001, uint(2), 0,
      35, QMetaType::Bool, 0x00015001, uint(3), 0,
      36, QMetaType::Bool, 0x00015001, uint(3), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject atrad::OrderController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_atrad__OrderController.offsetsAndSizes,
    qt_meta_data_atrad__OrderController,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_atrad__OrderController_t,
        // property 'instrumentId'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'price'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'volume'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'estimatedMargin'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'estimatedCommission'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'isAutoPrice'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'isTestMode'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'bidPrices'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'bidVolumes'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'askPrices'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'askVolumes'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'coreConnected'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'ctpConnected'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<OrderController, std::true_type>,
        // method 'orderParamsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'calculationChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'marketDataChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'connectionChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'orderSent'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onTick'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'updateInstrument'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'sendOrder'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'subscribe'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'unsubscribe'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'setPriceOriginal'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'setManualPrice'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'updateConnectionStatus'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'sendCommand'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
    >,
    nullptr
} };

void atrad::OrderController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<OrderController *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->orderParamsChanged(); break;
        case 1: _t->calculationChanged(); break;
        case 2: _t->marketDataChanged(); break;
        case 3: _t->connectionChanged(); break;
        case 4: _t->orderSent((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->onTick((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 6: _t->updateInstrument((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 7: _t->sendOrder((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 8: _t->subscribe((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 9: _t->unsubscribe((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 10: _t->setPriceOriginal((*reinterpret_cast< std::add_pointer_t<double>>(_a[1]))); break;
        case 11: _t->setManualPrice((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 12: _t->updateConnectionStatus((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2]))); break;
        case 13: _t->sendCommand((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (OrderController::*)();
            if (_t _q_method = &OrderController::orderParamsChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (OrderController::*)();
            if (_t _q_method = &OrderController::calculationChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (OrderController::*)();
            if (_t _q_method = &OrderController::marketDataChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (OrderController::*)();
            if (_t _q_method = &OrderController::connectionChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (OrderController::*)(const QString & );
            if (_t _q_method = &OrderController::orderSent; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
    }else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<OrderController *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->instrumentId(); break;
        case 1: *reinterpret_cast< double*>(_v) = _t->price(); break;
        case 2: *reinterpret_cast< int*>(_v) = _t->volume(); break;
        case 3: *reinterpret_cast< double*>(_v) = _t->estimatedMargin(); break;
        case 4: *reinterpret_cast< double*>(_v) = _t->estimatedCommission(); break;
        case 5: *reinterpret_cast< bool*>(_v) = _t->isAutoPrice(); break;
        case 6: *reinterpret_cast< bool*>(_v) = _t->isTestMode(); break;
        case 7: *reinterpret_cast< QVariantList*>(_v) = _t->bidPrices(); break;
        case 8: *reinterpret_cast< QVariantList*>(_v) = _t->bidVolumes(); break;
        case 9: *reinterpret_cast< QVariantList*>(_v) = _t->askPrices(); break;
        case 10: *reinterpret_cast< QVariantList*>(_v) = _t->askVolumes(); break;
        case 11: *reinterpret_cast< bool*>(_v) = _t->coreConnected(); break;
        case 12: *reinterpret_cast< bool*>(_v) = _t->ctpConnected(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<OrderController *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setInstrumentId(*reinterpret_cast< QString*>(_v)); break;
        case 1: _t->setPrice(*reinterpret_cast< double*>(_v)); break;
        case 2: _t->setVolume(*reinterpret_cast< int*>(_v)); break;
        case 5: _t->setAutoPrice(*reinterpret_cast< bool*>(_v)); break;
        case 6: _t->setTestMode(*reinterpret_cast< bool*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    } else if (_c == QMetaObject::BindableProperty) {
    }
}

const QMetaObject *atrad::OrderController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *atrad::OrderController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_atrad__OrderController.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int atrad::OrderController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 14;
    }else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    }
    return _id;
}

// SIGNAL 0
void atrad::OrderController::orderParamsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void atrad::OrderController::calculationChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void atrad::OrderController::marketDataChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void atrad::OrderController::connectionChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void atrad::OrderController::orderSent(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
