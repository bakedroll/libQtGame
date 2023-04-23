#include <libQtGame/KeyboardMouseEventFilter.h>

#include <QMouseEvent>
#include <QCursor>

#include <utilsLib/Utils.h>

namespace libQtGame
{

KeyboardMouseEventFilter::KeyboardMouseEventFilter(QObject* parent) :
  QObject(parent),
  m_isMouseCaptured(false)
{
}

KeyboardMouseEventFilter::~KeyboardMouseEventFilter() = default;

bool KeyboardMouseEventFilter::isKeyDown(Qt::Key key) const
{
  QMutexLocker locker(&m_mutex);

  if (m_isKeyDown.count(key) == 0)
  {
    return false;
  }
  return m_isKeyDown.find(key)->second;
}

bool KeyboardMouseEventFilter::isMouseButtonDown(Qt::MouseButton button) const
{
  QMutexLocker locker(&m_mutex);

  if (m_isMouseDown.count(button) == 0)
  {
    return false;
  }
  return m_isMouseDown.find(button)->second;
}

bool KeyboardMouseEventFilter::isMouseDragging(const std::optional<Qt::MouseButton>& button) const
{
  QMutexLocker locker(&m_mutex);
  return m_mouseDragData && m_mouseDragData->moved && (!button || (*button == m_mouseDragData->button));
}

void KeyboardMouseEventFilter::setCaptureMouse(bool on)
{
  QMutexLocker locker(&m_mutex);

  if (m_isMouseCaptured == on)
  {
    return;
  }

  m_isMouseCaptured  = on;
  m_capturedMousePos = QCursor::pos();
}

bool KeyboardMouseEventFilter::eventFilter(QObject* object, QEvent* event)
{
  if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease)
  {
    const auto mouseEvent = dynamic_cast<QMouseEvent*>(event);
    assert_return(mouseEvent, false);

    QMutexLocker locker(&m_mutex);
    m_isMouseDown[static_cast<Qt::MouseButton>(mouseEvent->button())] = (event->type() == QEvent::Type::MouseButtonPress);
  }

  switch (event->type())
  {
  case QEvent::Type::KeyPress:
  case QEvent::Type::KeyRelease:
  {
    const auto keyEvent = dynamic_cast<QKeyEvent*>(event);
    assert_return(keyEvent, false);

    QMutexLocker locker(&m_mutex);
    m_isKeyDown[static_cast<Qt::Key>(keyEvent->key())] = (event->type() == QEvent::Type::KeyPress);

    auto accepted = false;
    Q_EMIT triggerKeyEvent(keyEvent, accepted);
    return accepted;
  }
  case QEvent::Type::MouseButtonPress:
  case QEvent::Type::MouseButtonRelease:
  case QEvent::Type::MouseButtonDblClick:
  case QEvent::Type::MouseMove:
  {
    const auto mouseEvent = dynamic_cast<QMouseEvent*>(event);
    assert_return(mouseEvent, false);

    return handleMouseEvent(mouseEvent);
  }
  case QEvent::Type::HoverMove:
  {
    const auto hoverEvent = dynamic_cast<QHoverEvent*>(event);
    assert_return(hoverEvent, false);

    QMouseEvent mouseEvent(QEvent::MouseMove, hoverEvent->pos(),
      Qt::MouseButton::NoButton, Qt::MouseButton::NoButton, Qt::KeyboardModifier::NoModifier);

    return handleMouseEvent(&mouseEvent);
  }
  case QEvent::Type::Wheel:
  {
    const auto wheelEvent = dynamic_cast<QWheelEvent*>(event);
    assert_return(wheelEvent, false);

    auto accepted = false;
    Q_EMIT triggerWheelEvent(wheelEvent, accepted);
    return accepted;
  }
  default:
    break;
  }

  return false;
}

bool KeyboardMouseEventFilter::handleMouseEvent(QMouseEvent* mouseEvent)
{
  QMutexLocker locker(&m_mutex);
  switch (mouseEvent->type())
  {
  case QEvent::Type::MouseButtonPress:
  {
    if (!m_mouseDragData)
    {
      const osg::Vec2f origin(static_cast<float>(mouseEvent->pos().x()), static_cast<float>(mouseEvent->pos().y()));
      m_mouseDragData = MouseDragData{ mouseEvent->button(), false, origin, origin };
    }

    break;
  }
  case QEvent::Type::MouseMove:
  {
    if (!m_mouseDragData)
    {
      break;
    }

    if (!m_mouseDragData->moved)
    {
      Q_EMIT triggerDragBegin(m_mouseDragData->button, m_mouseDragData->origin);
      m_mouseDragData->moved = true;
    }

    const osg::Vec2f pos(static_cast<float>(mouseEvent->pos().x()), static_cast<float>(mouseEvent->pos().y()));

    if (m_isMouseCaptured)
    {
      const auto delta = m_capturedMousePos - mouseEvent->globalPos();
      Q_EMIT triggerDragMove(m_mouseDragData->button, m_mouseDragData->origin,
        pos, osg::Vec2f(static_cast<int>(delta.x()), static_cast<int>(delta.y())));
    }
    else
    {
      Q_EMIT triggerDragMove(m_mouseDragData->button, m_mouseDragData->origin,
        pos, m_mouseDragData->lastPos - pos);
    }

    m_mouseDragData->lastPos = pos;

    break;
  }
  case QEvent::Type::MouseButtonRelease:
  {
    if (m_mouseDragData && (m_mouseDragData->button == mouseEvent->button()))
    {
      Q_EMIT triggerDragEnd(m_mouseDragData->button, m_mouseDragData->origin,
        osg::Vec2f(static_cast<float>(mouseEvent->pos().x()), static_cast<float>(mouseEvent->pos().y())));

      m_mouseDragData.reset();
    }

    break;
  }
  default:
    break;
  }

  if (mouseEvent->type() == QEvent::Type::MouseMove && m_isMouseCaptured)
  {
    QCursor::setPos(m_capturedMousePos);
  }

  auto accepted = false;
  Q_EMIT triggerMouseEvent(mouseEvent, accepted);
  return accepted;
}

}
