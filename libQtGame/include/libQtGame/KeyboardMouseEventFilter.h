#pragma once

#include <QKeyEvent>
#include <QObject>
#include <QRecursiveMutex>

#include <osg/Vec2f>

#include <map>
#include <optional>

namespace libQtGame
{

class KeyboardMouseEventFilter : public QObject
{
  Q_OBJECT

public:
  explicit KeyboardMouseEventFilter(QObject* parent = nullptr);
  ~KeyboardMouseEventFilter() override;

  bool isKeyDown(Qt::Key key) const;
  bool isMouseButtonDown(Qt::MouseButton button) const;
  bool isMouseDragging(const std::optional<Qt::MouseButton>& button = std::nullopt) const;

  void setCaptureMouse(bool on);

Q_SIGNALS:
  void triggerKeyEvent(QKeyEvent* event, bool& accepted);
  void triggerMouseEvent(QMouseEvent* event, bool& accepted);
  void triggerWheelEvent(QWheelEvent* event, bool& accepted);

  void triggerDragBegin(Qt::MouseButton button, const osg::Vec2f& origin);
  void triggerDragMove(Qt::MouseButton button, const osg::Vec2f& origin, const osg::Vec2f& position, const osg::Vec2f& change);
  void triggerDragEnd(Qt::MouseButton button, const osg::Vec2f& origin, const osg::Vec2f& position);

protected:
  bool eventFilter(QObject* object, QEvent* event) override;

private:
  struct MouseDragData
  {
    Qt::MouseButton button;
    bool            moved;
    osg::Vec2f      origin;
    osg::Vec2f      lastPos;
  };

  mutable QRecursiveMutex m_mutex;

  std::optional<MouseDragData> m_mouseDragData;

  std::map<Qt::MouseButton, bool> m_isMouseDown;
  std::map<Qt::Key, bool>         m_isKeyDown;

  bool m_isMouseCaptured;
  QPoint m_capturedMousePos;

  bool handleMouseEvent(QMouseEvent* mouseEvent);

};

}
