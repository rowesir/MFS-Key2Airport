/****************************************************************************
** Meta object code from reading C++ file 'simconnectclient.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.7.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../simconnectclient.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'simconnectclient.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.7.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {

#ifdef QT_MOC_HAS_STRINGDATA
struct qt_meta_stringdata_CLASSSimConnectClientENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSSimConnectClientENDCLASS = QtMocHelpers::stringData(
    "SimConnectClient",
    "connected",
    "",
    "disconnected",
    "connectionLost",
    "flightStarted",
    "aircraftLoaded",
    "file",
    "flightEnded",
    "inputEventEnumerated",
    "name",
    "hash",
    "eType",
    "inputEventParamsEnumerated",
    "param",
    "size",
    "inputEventReceived",
    "value",
    "inputEventValueReceived",
    "inputEventValueUnavailable",
    "radioHeightReceived",
    "radioHeightUnavailable",
    "landingRateReceived",
    "feetPerMinute",
    "landingRateCleared",
    "landingRateUnavailable",
    "simError",
    "code",
    "connectToSim",
    "disconnectFromSim",
    "enumerateInputEvents",
    "stopInputEventListening",
    "getInputEvent",
    "sendInputEvent",
    "startRadioHeightReading",
    "stopRadioHeightReading",
    "startLandingRateReading",
    "stopLandingRateReading"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSSimConnectClientENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      27,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      17,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  176,    2, 0x06,    1 /* Public */,
       3,    0,  177,    2, 0x06,    2 /* Public */,
       4,    0,  178,    2, 0x06,    3 /* Public */,
       5,    0,  179,    2, 0x06,    4 /* Public */,
       6,    1,  180,    2, 0x06,    5 /* Public */,
       8,    0,  183,    2, 0x06,    7 /* Public */,
       9,    3,  184,    2, 0x06,    8 /* Public */,
      13,    3,  191,    2, 0x06,   12 /* Public */,
      16,    5,  198,    2, 0x06,   16 /* Public */,
      18,    2,  209,    2, 0x06,   22 /* Public */,
      19,    1,  214,    2, 0x06,   25 /* Public */,
      20,    1,  217,    2, 0x06,   27 /* Public */,
      21,    0,  220,    2, 0x06,   29 /* Public */,
      22,    1,  221,    2, 0x06,   30 /* Public */,
      24,    0,  224,    2, 0x06,   32 /* Public */,
      25,    0,  225,    2, 0x06,   33 /* Public */,
      26,    1,  226,    2, 0x06,   34 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      28,    0,  229,    2, 0x0a,   36 /* Public */,
      29,    0,  230,    2, 0x0a,   37 /* Public */,
      30,    0,  231,    2, 0x0a,   38 /* Public */,
      31,    0,  232,    2, 0x0a,   39 /* Public */,
      32,    1,  233,    2, 0x0a,   40 /* Public */,
      33,    2,  236,    2, 0x0a,   42 /* Public */,
      34,    0,  241,    2, 0x0a,   45 /* Public */,
      35,    0,  242,    2, 0x0a,   46 /* Public */,
      36,    0,  243,    2, 0x0a,   47 /* Public */,
      37,    0,  244,    2, 0x0a,   48 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::ULongLong, QMetaType::QString,   10,   11,   12,
    QMetaType::Void, QMetaType::ULongLong, QMetaType::QString, QMetaType::Int,   11,   14,   15,
    QMetaType::Void, QMetaType::ULongLong, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::Int,   11,   12,   17,   14,   15,
    QMetaType::Void, QMetaType::ULongLong, QMetaType::Double,   11,   17,
    QMetaType::Void, QMetaType::ULongLong,   11,
    QMetaType::Void, QMetaType::Double,   17,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Double,   23,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::UInt,   27,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::ULongLong,   11,
    QMetaType::Void, QMetaType::ULongLong, QMetaType::Double,   11,   17,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject SimConnectClient::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSSimConnectClientENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSSimConnectClientENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSSimConnectClientENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<SimConnectClient, std::true_type>,
        // method 'connected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'disconnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'connectionLost'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'flightStarted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'aircraftLoaded'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'flightEnded'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'inputEventEnumerated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<quint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'inputEventParamsEnumerated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<quint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'inputEventReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<quint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'inputEventValueReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<quint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'inputEventValueUnavailable'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<quint64, std::false_type>,
        // method 'radioHeightReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'radioHeightUnavailable'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'landingRateReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'landingRateCleared'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'landingRateUnavailable'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'simError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<quint32, std::false_type>,
        // method 'connectToSim'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'disconnectFromSim'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'enumerateInputEvents'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'stopInputEventListening'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'getInputEvent'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<quint64, std::false_type>,
        // method 'sendInputEvent'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<quint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'startRadioHeightReading'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'stopRadioHeightReading'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'startLandingRateReading'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'stopLandingRateReading'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void SimConnectClient::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SimConnectClient *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->connected(); break;
        case 1: _t->disconnected(); break;
        case 2: _t->connectionLost(); break;
        case 3: _t->flightStarted(); break;
        case 4: _t->aircraftLoaded((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->flightEnded(); break;
        case 6: _t->inputEventEnumerated((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<quint64>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 7: _t->inputEventParamsEnumerated((*reinterpret_cast< std::add_pointer_t<quint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[3]))); break;
        case 8: _t->inputEventReceived((*reinterpret_cast< std::add_pointer_t<quint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[4])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[5]))); break;
        case 9: _t->inputEventValueReceived((*reinterpret_cast< std::add_pointer_t<quint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[2]))); break;
        case 10: _t->inputEventValueUnavailable((*reinterpret_cast< std::add_pointer_t<quint64>>(_a[1]))); break;
        case 11: _t->radioHeightReceived((*reinterpret_cast< std::add_pointer_t<double>>(_a[1]))); break;
        case 12: _t->radioHeightUnavailable(); break;
        case 13: _t->landingRateReceived((*reinterpret_cast< std::add_pointer_t<double>>(_a[1]))); break;
        case 14: _t->landingRateCleared(); break;
        case 15: _t->landingRateUnavailable(); break;
        case 16: _t->simError((*reinterpret_cast< std::add_pointer_t<quint32>>(_a[1]))); break;
        case 17: _t->connectToSim(); break;
        case 18: _t->disconnectFromSim(); break;
        case 19: _t->enumerateInputEvents(); break;
        case 20: _t->stopInputEventListening(); break;
        case 21: _t->getInputEvent((*reinterpret_cast< std::add_pointer_t<quint64>>(_a[1]))); break;
        case 22: _t->sendInputEvent((*reinterpret_cast< std::add_pointer_t<quint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[2]))); break;
        case 23: _t->startRadioHeightReading(); break;
        case 24: _t->stopRadioHeightReading(); break;
        case 25: _t->startLandingRateReading(); break;
        case 26: _t->stopLandingRateReading(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SimConnectClient::*)();
            if (_t _q_method = &SimConnectClient::connected; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (SimConnectClient::*)();
            if (_t _q_method = &SimConnectClient::disconnected; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (SimConnectClient::*)();
            if (_t _q_method = &SimConnectClient::connectionLost; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (SimConnectClient::*)();
            if (_t _q_method = &SimConnectClient::flightStarted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (SimConnectClient::*)(const QString & );
            if (_t _q_method = &SimConnectClient::aircraftLoaded; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (SimConnectClient::*)();
            if (_t _q_method = &SimConnectClient::flightEnded; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (SimConnectClient::*)(const QString & , quint64 , const QString & );
            if (_t _q_method = &SimConnectClient::inputEventEnumerated; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (SimConnectClient::*)(quint64 , const QString & , int );
            if (_t _q_method = &SimConnectClient::inputEventParamsEnumerated; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (SimConnectClient::*)(quint64 , const QString & , const QString & , const QString & , int );
            if (_t _q_method = &SimConnectClient::inputEventReceived; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (SimConnectClient::*)(quint64 , double );
            if (_t _q_method = &SimConnectClient::inputEventValueReceived; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (SimConnectClient::*)(quint64 );
            if (_t _q_method = &SimConnectClient::inputEventValueUnavailable; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (SimConnectClient::*)(double );
            if (_t _q_method = &SimConnectClient::radioHeightReceived; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (SimConnectClient::*)();
            if (_t _q_method = &SimConnectClient::radioHeightUnavailable; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 12;
                return;
            }
        }
        {
            using _t = void (SimConnectClient::*)(double );
            if (_t _q_method = &SimConnectClient::landingRateReceived; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 13;
                return;
            }
        }
        {
            using _t = void (SimConnectClient::*)();
            if (_t _q_method = &SimConnectClient::landingRateCleared; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 14;
                return;
            }
        }
        {
            using _t = void (SimConnectClient::*)();
            if (_t _q_method = &SimConnectClient::landingRateUnavailable; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 15;
                return;
            }
        }
        {
            using _t = void (SimConnectClient::*)(quint32 );
            if (_t _q_method = &SimConnectClient::simError; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 16;
                return;
            }
        }
    }
}

const QMetaObject *SimConnectClient::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SimConnectClient::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSSimConnectClientENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SimConnectClient::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 27)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 27;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 27)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 27;
    }
    return _id;
}

// SIGNAL 0
void SimConnectClient::connected()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void SimConnectClient::disconnected()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void SimConnectClient::connectionLost()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void SimConnectClient::flightStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void SimConnectClient::aircraftLoaded(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void SimConnectClient::flightEnded()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void SimConnectClient::inputEventEnumerated(const QString & _t1, quint64 _t2, const QString & _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void SimConnectClient::inputEventParamsEnumerated(quint64 _t1, const QString & _t2, int _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void SimConnectClient::inputEventReceived(quint64 _t1, const QString & _t2, const QString & _t3, const QString & _t4, int _t5)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t5))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void SimConnectClient::inputEventValueReceived(quint64 _t1, double _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void SimConnectClient::inputEventValueUnavailable(quint64 _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void SimConnectClient::radioHeightReceived(double _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void SimConnectClient::radioHeightUnavailable()
{
    QMetaObject::activate(this, &staticMetaObject, 12, nullptr);
}

// SIGNAL 13
void SimConnectClient::landingRateReceived(double _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 13, _a);
}

// SIGNAL 14
void SimConnectClient::landingRateCleared()
{
    QMetaObject::activate(this, &staticMetaObject, 14, nullptr);
}

// SIGNAL 15
void SimConnectClient::landingRateUnavailable()
{
    QMetaObject::activate(this, &staticMetaObject, 15, nullptr);
}

// SIGNAL 16
void SimConnectClient::simError(quint32 _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 16, _a);
}
QT_WARNING_POP
