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
    setMouseDown(mouseEvent->button(), event->type() == QEvent::Type::MouseButtonPress);
  }

  switch (event->type())
  {
  case QEvent::Type::KeyPress:
  case QEvent::Type::KeyRelease:
  {
    const auto keyEvent = dynamic_cast<QKeyEvent*>(event);
    assert_return(keyEvent, false);

    if (keyEvent->isAutoRepeat())
    {
      break;
    }
    setKeyDown(static_cast<Qt::Key>(keyEvent->key()), event->type() == QEvent::Type::KeyPress);

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

void KeyboardMouseEventFilter::setMouseDown(Qt::MouseButton button, bool down)
{
  QMutexLocker locker(&m_mutex);
  m_isMouseDown[button] = down;
}

void KeyboardMouseEventFilter::setKeyDown(Qt::Key key, bool down)
{
  QMutexLocker locker(&m_mutex);
  m_isKeyDown[key] = down;
}

bool KeyboardMouseEventFilter::handleMouseEvent(QMouseEvent* mouseEvent)
{
  switch (mouseEvent->type())
  {
  case QEvent::Type::MouseButtonPress:
  {
    handleMouseButtonPress(mouseEvent);
    break;
  }
  case QEvent::Type::MouseMove:
  {
    const auto data = handleMouseDragMove(mouseEvent);
    if (data.valid)
    {
      if (data.isBegin)
      {
        Q_EMIT triggerDragBegin(data.button, data.origin);
      }
      Q_EMIT triggerDragMove(data.button, data.origin, data.position, data.change);
    }

    break;
  }
  case QEvent::Type::MouseButtonRelease:
  {
    const auto dragData = handleMouseButtonRelease(mouseEvent);
    if (dragData.has_value())
    {
      Q_EMIT triggerDragEnd(dragData->button, dragData->origin,
        osg::Vec2f(static_cast<float>(mouseEvent->pos().x()), static_cast<float>(mouseEvent->pos().y())));
    }
    break;
  }
  default:
    break;
  }

  handleMouseCapture(mouseEvent);

  auto accepted = false;
  Q_EMIT triggerMouseEvent(mouseEvent, accepted);
  return accepted;
}

void KeyboardMouseEventFilter::handleMouseButtonPress(QMouseEvent* mouseEvent)
{
  QMutexLocker locker(&m_mutex);
  if (!m_mouseDragData)
  {
    const osg::Vec2f origin(static_cast<float>(mouseEvent->pos().x()), static_cast<float>(mouseEvent->pos().y()));
    m_mouseDragData = MouseDragData{ mouseEvent->button(), false, origin, origin };
  }
}

std::optional<KeyboardMouseEventFilter::MouseDragData> KeyboardMouseEventFilter::handleMouseButtonRelease(QMouseEvent* mouseEvent)
{
  QMutexLocker locker(&m_mutex);
  if (m_mouseDragData && (m_mouseDragData->button == mouseEvent->button()))
  {
    const auto result = m_mouseDragData;
    m_mouseDragData.reset();
    return result;
  }

  return std::nullopt;
}

void KeyboardMouseEventFilter::handleMouseCapture(QMouseEvent* mouseEvent)
{
  QMutexLocker locker(&m_mutex);
  if (mouseEvent->type() == QEvent::Type::MouseMove && m_isMouseCaptured)
  {
    QCursor::setPos(m_capturedMousePos);
  }
}

KeyboardMouseEventFilter::MouseDragMoveData KeyboardMouseEventFilter::handleMouseDragMove(QMouseEvent* mouseEvent)
{
  QMutexLocker locker(&m_mutex);
  if (!m_mouseDragData)
  {
    return {};
  }

  const osg::Vec2f pos(static_cast<float>(mouseEvent->pos().x()), static_cast<float>(mouseEvent->pos().y()));
  MouseDragMoveData data { true, !m_mouseDragData->moved, m_mouseDragData->button, m_mouseDragData->origin, pos, osg::Vec2f() };

  if (!m_mouseDragData->moved)
  {
    m_mouseDragData->moved = true;
  }

  if (m_isMouseCaptured)
  {
    const auto delta = m_capturedMousePos - mouseEvent->globalPos();
    data.change = osg::Vec2f(static_cast<int>(delta.x()), static_cast<int>(delta.y()));
  }
  else
  {
    data.change = m_mouseDragData->lastPos - pos;
  }

  m_mouseDragData->lastPos = pos;
  return data;
}

}
