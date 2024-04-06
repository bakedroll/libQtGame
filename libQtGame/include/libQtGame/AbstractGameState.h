#pragma once

#include <utilsLib/Utils.h>

#include <QtUtilsLib/Multithreading.h>

#include <osgHelper/ioc/Injector.h>
#include <osgHelper/SimulationCallback.h>

#include <QObject>

namespace libQtGame
{

class AbstractGameState : public QObject,
                          public osg::Referenced
{
  Q_OBJECT

public:
  enum class NewGameStateMode
  {
    ExitCurrent,
    ContinueCurrent
  };

  enum class ExitGameStateMode
  {
    ExitCurrent,
    ExitAll
  };

  using SimulationData = osgHelper::SimulationCallback::SimulationData;

  explicit AbstractGameState(osgHelper::ioc::Injector& injector);
  ~AbstractGameState() override;

  virtual void onInitialize(const SimulationData& data);
  virtual void onUpdate(const SimulationData& data);
  virtual void onExit();

  template <typename TState>
  void requestNewEventState(NewGameStateMode mode = NewGameStateMode::ContinueCurrent)
  {
    if (mode == NewGameStateMode::ExitCurrent)
    {
      m_isExiting = true;
    }

    QtUtilsLib::Multithreading::executeInUiAsync([this, mode]()
    {
      const auto state = m_injector->inject<TState>();
      assert_return(state.valid());

      Q_EMIT forwardNewEventStateRequest(this, mode, state);
    });
  }

  void requestExitEventState(ExitGameStateMode mode = ExitGameStateMode::ExitCurrent);
  void requestResetTimeDelta();

  bool isExiting() const;

Q_SIGNALS:
  void forwardNewEventStateRequest(const osg::ref_ptr<AbstractGameState>& current, NewGameStateMode mode,
                                   const osg::ref_ptr<AbstractGameState>& newState);
  void forwardExitEventStateRequest(const osg::ref_ptr<AbstractGameState>& current, ExitGameStateMode mode);

  void forwardResetTimeDeltaRequest();

private:
  osgHelper::ioc::Injector* m_injector;
  bool m_isExiting;

};

}
