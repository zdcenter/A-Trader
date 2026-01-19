/****************************************************************************
** Meta object code from reading C++ file 'MarketModel.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/models/MarketModel.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MarketModel.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_atrad__MarketModel_t {
    uint offsetsAndSizes[32];
    char stringdata0[19];
    char stringdata1[11];
    char stringdata2[1];
    char stringdata3[5];
    char stringdata4[17];
    char stringdata5[17];
    char stringdata6[13];
    char stringdata7[14];
    char stringdata8[5];
    char stringdata9[5];
    char stringdata10[3];
    char stringdata11[18];
    char stringdata12[10];
    char stringdata13[6];
    char stringdata14[13];
    char stringdata15[14];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_atrad__MarketModel_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_atrad__MarketModel_t qt_meta_stringdata_atrad__MarketModel = {
    {
        QT_MOC_LITERAL(0, 18),  // "atrad::MarketModel"
        QT_MOC_LITERAL(19, 10),  // "updateTick"
        QT_MOC_LITERAL(30, 0),  // ""
        QT_MOC_LITERAL(31, 4),  // "json"
        QT_MOC_LITERAL(36, 16),  // "handleInstrument"
        QT_MOC_LITERAL(53, 16),  // "removeInstrument"
        QT_MOC_LITERAL(70, 12),  // "instrumentId"
        QT_MOC_LITERAL(83, 13),  // "addInstrument"
        QT_MOC_LITERAL(97, 4),  // "move"
        QT_MOC_LITERAL(102, 4),  // "from"
        QT_MOC_LITERAL(107, 2),  // "to"
        QT_MOC_LITERAL(110, 17),  // "getAllInstruments"
        QT_MOC_LITERAL(128, 9),  // "moveToTop"
        QT_MOC_LITERAL(138, 5),  // "index"
        QT_MOC_LITERAL(144, 12),  // "moveToBottom"
        QT_MOC_LITERAL(157, 13)   // "hasInstrument"
    },
    "atrad::MarketModel",
    "updateTick",
    "",
    "json",
    "handleInstrument",
    "removeInstrument",
    "instrumentId",
    "addInstrument",
    "move",
    "from",
    "to",
    "getAllInstruments",
    "moveToTop",
    "index",
    "moveToBottom",
    "hasInstrument"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_atrad__MarketModel[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   68,    2, 0x0a,    1 /* Public */,
       4,    1,   71,    2, 0x0a,    3 /* Public */,
       5,    1,   74,    2, 0x0a,    5 /* Public */,
       7,    1,   77,    2, 0x0a,    7 /* Public */,
       8,    2,   80,    2, 0x0a,    9 /* Public */,
      11,    0,   85,    2, 0x10a,   12 /* Public | MethodIsConst  */,
      12,    1,   86,    2, 0x0a,   13 /* Public */,
      14,    1,   89,    2, 0x0a,   15 /* Public */,
      15,    1,   92,    2, 0x10a,   17 /* Public | MethodIsConst  */,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,    9,   10,
    QMetaType::QStringList,
    QMetaType::Void, QMetaType::Int,   13,
    QMetaType::Void, QMetaType::Int,   13,
    QMetaType::Bool, QMetaType::QString,    6,

       0        // eod
};

Q_CONSTINIT const QMetaObject atrad::MarketModel::staticMetaObject = { {
    QMetaObject::SuperData::link<QAbstractListModel::staticMetaObject>(),
    qt_meta_stringdata_atrad__MarketModel.offsetsAndSizes,
    qt_meta_data_atrad__MarketModel,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_atrad__MarketModel_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<MarketModel, std::true_type>,
        // method 'updateTick'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'handleInstrument'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'removeInstrument'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'addInstrument'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'move'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'getAllInstruments'
        QtPrivate::TypeAndForceComplete<QStringList, std::false_type>,
        // method 'moveToTop'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'moveToBottom'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'hasInstrument'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
    >,
    nullptr
} };

void atrad::MarketModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MarketModel *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->updateTick((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->handleInstrument((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->removeInstrument((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->addInstrument((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->move((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 5: { QStringList _r = _t->getAllInstruments();
            if (_a[0]) *reinterpret_cast< QStringList*>(_a[0]) = std::move(_r); }  break;
        case 6: _t->moveToTop((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 7: _t->moveToBottom((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 8: { bool _r = _t->hasInstrument((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
}

const QMetaObject *atrad::MarketModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *atrad::MarketModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_atrad__MarketModel.stringdata0))
        return static_cast<void*>(this);
    return QAbstractListModel::qt_metacast(_clname);
}

int atrad::MarketModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QAbstractListModel::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 9;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
