//****************************************************************************
// Product: QF/C++ port to Qt
// Last Updated for Version: 5.1.0
// Date of the Last Update:  Sep 28, 2013
//
//                    Q u a n t u m     L e a P s
//                    ---------------------------
//                    innovating embedded systems
//
// Copyright (C) 2002-2013 Quantum Leaps, LLC. All rights reserved.
//
// This program is open source software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Alternatively, this program may be distributed and modified under the
// terms of Quantum Leaps commercial licenses, which expressly supersede
// the GNU General Public License and are specifically designed for
// licensees interested in retaining the proprietary status of their code.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//
// Contact information:
// Quantum Leaps Web sites: http://www.quantum-leaps.com
//                          http://www.state-machine.com
// e-mail:                  info@quantum-leaps.com
//****************************************************************************
#include "qf_pkg.h"
#include "qassert.h"

Q_DEFINE_THIS_MODULE("guiactive")

static QEvent::Type l_qp_event_type = QEvent::MaxUser;

//****************************************************************************
class QP_Event : public QEvent {
public:
    QP_Event(QP::QEvt const *e)
      : QEvent(l_qp_event_type),
        m_qpevt(e)
    {}

    QP::QEvt const *m_qpevt;                               // QP event pointer
};

//****************************************************************************
GuiApp::GuiApp(int &argc, char **argv)
  : QApplication(argc, argv),
    m_act(0)
{
    l_qp_event_type = static_cast<QEvent::Type>(QEvent::registerEventType());
}
//............................................................................
void GuiApp::registerAct(void *act) {
    Q_REQUIRE(m_act == 0);     // the GUI active object must not be registered
    m_act = act;
}
//............................................................................
bool GuiApp::event(QEvent *e) {
    if (e->type() == l_qp_event_type) {
        QP::QEvt const *qpevt = (static_cast<QP_Event *>(e))->m_qpevt;
        static_cast<QP::QActive *>(m_act)->dispatch(qpevt);  // dispatch to AO
        QP::QF::gc(qpevt);                       // garbage collect the QP evt
        return true;                           // event recognized and handled
    }
    else {
        return QApplication::event(e);           // delegate to the superclass
    }
}

//****************************************************************************
namespace QP {

//............................................................................
void GuiQActive::start(uint8_t const prio,
                       QEvt const **qSto, uint32_t /*qLen*/,
                       void * const stkSto, uint32_t const /*stkSize*/,
                       QEvt const * const ie)
{
    Q_REQUIRE((u8_0 < prio) && (prio <= static_cast<uint8_t>(QF_MAX_ACTIVE))
              && (qSto == (QEvt const **)0)/* does not need per-actor queue */
              && (stkSto == null_void));      // does not need per-actor stack

    setPrio(prio);                // set the QF priority of this active object
    QF::add_(this);                     // make QF aware of this active object
    static_cast<GuiApp *>(QApplication::instance())->registerAct(this);

    this->init(ie);               // execute initial transition (virtual call)
    QS_FLUSH();                          // flush the trace buffer to the host
}
//............................................................................
#ifndef Q_SPY
bool GuiQActive::post(QEvt const * const e, uint16_t const /*margin*/)
#else
bool GuiQActive::post(QEvt const * const e, uint16_t const /*margin*/,
                      void const * const sender)
#endif
{
    QF_CRIT_STAT_
    QF_CRIT_ENTRY_();

    QS_BEGIN_NOCRIT_(QS_QF_ACTIVE_POST_FIFO, QS::priv_.aoObjFilter, this)
        QS_TIME_();                                               // timestamp
        QS_OBJ_(sender);                                  // the sender object
        QS_SIG_(e->sig);                            // the signal of the event
        QS_OBJ_(this);                                   // this active object
        QS_2U8_(QF_EVT_POOL_ID_(e),              /* the poolID of the event */
                QF_EVT_REF_CTR_(e));               // the ref Ctr of the event
        QS_EQC_(0);                       // number of free entries (not used)
        QS_EQC_(0);                   // min number of free entries (not used)
    QS_END_NOCRIT_()

    if (QF_EVT_POOL_ID_(e) != u8_0) {                // is it a dynamic event?
        QF_EVT_REF_CTR_INC_(e);             // increment the reference counter
    }
    QF_CRIT_EXIT_();

    // QCoreApplication::postEvent() is thread-safe per Qt documentation
    QCoreApplication::postEvent(QApplication::instance(), new QP_Event(e));
    return true;
}
//............................................................................
void GuiQActive::postLIFO(QEvt const * const e) {
    QF_CRIT_STAT_
    QF_CRIT_ENTRY_();

    QS_BEGIN_NOCRIT_(QS_QF_ACTIVE_POST_LIFO, QS::priv_.aoObjFilter, this)
        QS_TIME_();                                               // timestamp
        QS_SIG_(e->sig);                           // the signal of this event
        QS_OBJ_(this);                                   // this active object
        QS_2U8_(QF_EVT_POOL_ID_(e),              /* the poolID of the event */
                QF_EVT_REF_CTR_(e));               // the ref Ctr of the event
        QS_EQC_(0);                       // number of free entries (not used)
        QS_EQC_(0);                   // min number of free entries (not used)
    QS_END_NOCRIT_()

    if (QF_EVT_POOL_ID_(e) != u8_0) {                // is it a dynamic event?
        QF_EVT_REF_CTR_INC_(e);             // increment the reference counter
    }
    QF_CRIT_EXIT_();

    // QCoreApplication::postEvent() is thread-safe per Qt documentation
    QCoreApplication::postEvent(QApplication::instance(), new QP_Event(e),
                                Qt::HighEventPriority);
}

//****************************************************************************
void GuiQMActive::start(uint8_t const prio,
                        QEvt const **qSto, uint32_t /*qLen*/,
                        void * const stkSto, uint32_t const /*stkSize*/,
                        QEvt const * const ie)
{
    Q_REQUIRE((u8_0 < prio) && (prio <= static_cast<uint8_t>(QF_MAX_ACTIVE))
              && (qSto == (QEvt const **)0)/* does not need per-actor queue */
              && (stkSto == null_void));      // does not need per-actor stack

    setPrio(prio);                // set the QF priority of this active object
    QF::add_(this);                     // make QF aware of this active object
    static_cast<GuiApp *>(QApplication::instance())->registerAct(this);

    this->init(ie);               // execute initial transition (virtual call)
    QS_FLUSH();                          // flush the trace buffer to the host
}
//............................................................................
#ifndef Q_SPY
bool GuiQMActive::post(QEvt const * const e, uint16_t const /*margin*/)
#else
bool GuiQMActive::post(QEvt const * const e, uint16_t const /*margin*/,
                       void const * const sender)
#endif
{
    QF_CRIT_STAT_
    QF_CRIT_ENTRY_();

    QS_BEGIN_NOCRIT_(QS_QF_ACTIVE_POST_FIFO, QS::priv_.aoObjFilter, this)
        QS_TIME_();                                               // timestamp
        QS_OBJ_(sender);                                  // the sender object
        QS_SIG_(e->sig);                            // the signal of the event
        QS_OBJ_(this);                                   // this active object
        QS_2U8_(QF_EVT_POOL_ID_(e),              /* the poolID of the event */
                QF_EVT_REF_CTR_(e));               // the ref Ctr of the event
        QS_EQC_(0);                       // number of free entries (not used)
        QS_EQC_(0);                   // min number of free entries (not used)
    QS_END_NOCRIT_()

    if (QF_EVT_POOL_ID_(e) != u8_0) {                // is it a dynamic event?
        QF_EVT_REF_CTR_INC_(e);             // increment the reference counter
    }
    QF_CRIT_EXIT_();

    // QCoreApplication::postEvent() is thread-safe per Qt documentation
    QCoreApplication::postEvent(QApplication::instance(), new QP_Event(e));
    return true;
}
//............................................................................
void GuiQMActive::postLIFO(QEvt const * const e) {
    QF_CRIT_STAT_
    QF_CRIT_ENTRY_();

    QS_BEGIN_NOCRIT_(QS_QF_ACTIVE_POST_LIFO, QS::priv_.aoObjFilter, this)
        QS_TIME_();                                               // timestamp
        QS_SIG_(e->sig);                           // the signal of this event
        QS_OBJ_(this);                                   // this active object
        QS_2U8_(QF_EVT_POOL_ID_(e),              /* the poolID of the event */
                QF_EVT_REF_CTR_(e));               // the ref Ctr of the event
        QS_EQC_(0);                       // number of free entries (not used)
        QS_EQC_(0);                   // min number of free entries (not used)
    QS_END_NOCRIT_()

    if (QF_EVT_POOL_ID_(e) != u8_0) {                // is it a dynamic event?
        QF_EVT_REF_CTR_INC_(e);             // increment the reference counter
    }
    QF_CRIT_EXIT_();

    // QCoreApplication::postEvent() is thread-safe per Qt documentation
    QCoreApplication::postEvent(QApplication::instance(), new QP_Event(e),
                                Qt::HighEventPriority);
}

}                                                              // namespace QP