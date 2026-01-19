/****************************************************************************
** Meta object code from reading C++ file 'AccountInfo.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/models/AccountInfo.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AccountInfo.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_atrad__AccountInfo_t {
    uint offsetsAndSizes[28];
    char stringdata0[19];
    char stringdata1[8];
    char stringdata2[1];
    char stringdata3[14];
    char stringdata4[5];
    char stringdata5[18];
    char stringdata6[7];
    char stringdata7[8];
    char stringdata8[10];
    char stringdata9[7];
    char stringdata10[7];
    char stringdata11[11];
    char stringdata12[15];
    char stringdata13[7];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_atrad__AccountInfo_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_atrad__AccountInfo_t qt_meta_stringdata_atrad__AccountInfo = {
    {
        QT_MOC_LITERAL(0, 18),  // "atrad::AccountInfo"
        QT_MOC_LITERAL(19, 7),  // "changed"
        QT_MOC_LITERAL(27, 0),  // ""
        QT_MOC_LITERAL(28, 13),  // "updateAccount"
        QT_MOC_LITERAL(42, 4),  // "json"
        QT_MOC_LITERAL(47, 17),  // "setFloatingProfit"
        QT_MOC_LITERAL(65, 6),  // "profit"
        QT_MOC_LITERAL(72, 7),  // "balance"
        QT_MOC_LITERAL(80, 9),  // "available"
        QT_MOC_LITERAL(90, 6),  // "margin"
        QT_MOC_LITERAL(97, 6),  // "frozen"
        QT_MOC_LITERAL(104, 10),  // "commission"
        QT_MOC_LITERAL(115, 14),  // "floatingProfit"
        QT_MOC_LITERAL(130, 6)   // "equity"
    },
    "atrad::AccountInfo",
    "changed",
    "",
    "updateAccount",
    "json",
    "setFloatingProfit",
    "profit",
    "balance",
    "available",
    "margin",
    "frozen",
    "commission",
    "floatingProfit",
    "equity"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_atrad__AccountInfo[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       7,   39, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   32,    2, 0x06,    8 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       3,    1,   33,    2, 0x0a,    9 /* Public */,
       5,    1,   36,    2, 0x0a,   11 /* Public */,

 // signals: parameters
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::Double,    6,

 // properties: name, type, flags
       7, QMetaType::Double, 0x00015001, uint(0), 0,
       8, QMetaType::Double, 0x00015001, uint(0), 0,
       9, QMetaType::Double, 0x00015001, uint(0), 0,
      10, QMetaType::Double, 0x00015001, uint(0), 0,
      11, QMetaType::Double, 0x00015001, uint(0), 0,
      12, QMetaType::Double, 0x00015001, uint(0), 0,
      13, QMetaType::Double, 0x00015001, uint(0), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject atrad::AccountInfo::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_atrad__AccountInfo.offsetsAndSizes,
    qt_meta_data_atrad__AccountInfo,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_atrad__AccountInfo_t,
        // property 'balance'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'available'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'margin'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'frozen'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'commission'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'floatingProfit'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'equity'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<AccountInfo, std::true_type>,
        // method 'changed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateAccount'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'setFloatingProfit'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>
    >,
    nullptr
} };

void atrad::AccountInfo::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<AccountInfo *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->changed(); break;
        case 1: _t->updateAccount((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->setFloatingProfit((*reinterpret_cast< std::add_pointer_t<double>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (AccountInfo::*)();
            if (_t _q_method = &AccountInfo::changed; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    }else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<AccountInfo *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< double*>(_v) = _t->balance(); break;
        case 1: *reinterpret_cast< double*>(_v) = _t->available(); break;
        case 2: *reinterpret_cast< double*>(_v) = _t->margin(); break;
        case 3: *reinterpret_cast< double*>(_v) = _t->frozen(); break;
        case 4: *reinterpret_cast< double*>(_v) = _t->commission(); break;
        case 5: *reinterpret_cast< double*>(_v) = _t->floatingProfit(); break;
        case 6: *reinterpret_cast< double*>(_v) = _t->equity(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
    } else if (_c == QMetaObject::ResetProperty) {
    } else if (_c == QMetaObject::BindableProperty) {
    }
}

const QMetaObject *atrad::AccountInfo::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *atrad::AccountInfo::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_atrad__AccountInfo.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int atrad::AccountInfo::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 3;
    }else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void atrad::AccountInfo::changed()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
