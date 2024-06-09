#pragma once

#include <libQtGame/AbstractGameState.h>
#include <libQtGame/GameUpdateCallback.h>

#include <QMetaObject>
#include <QRecursiveMutex>

#include <QtUtilsLib/QtUtilsApplication.h>

#include <osgHelper/GameApplication.h>
#include <osgHelper/ioc/InjectionContainer.h>
#include <osgHelper/SimulationCallback.h>

#include <memory>

namespace libQtGame
{

class GameStatesObject;

class GameStatesApplication : public QtUtilsLib::QtUtilsApplication<osg::ref_ptr<osg::Referenced>>,
                              public osgHelper::GameApplication
{
public:
  GameStatesApplication();
  ~GameStatesApplication();

protected:
  struct StateData
  {
    osg::ref_ptr<AbstractGameState> state;
    std::vector<QMetaObject::Connection> connections;
  };

  int runGame();

  void prepareGameState(StateData& data);
  void onException(const std::string& message) override;

  virtual void registerEssentialComponents(osgHelper::ioc::InjectionContainer& container);

  virtual void onInitialize(const osg::ref_ptr<libQtGame::GameUpdateCallback>& updateCallback) = 0;
  virtual void onPrepareGameState(
    const osg::ref_ptr<AbstractGameState>& state,
    const AbstractGameState::SimulationData& simData) = 0;
  virtual void onExitGameState(const osg::ref_ptr<AbstractGameState>& state) = 0;
  virtual void onEmptyStateList() = 0;
  virtual void onShutdown() = 0;
  virtual void onPreStatesUpdate(const osgHelper::SimulationCallback::SimulationData& data);

  template <typename TState>
  bool injectPushAndPrepareState()
  {
    QMutexLocker locker(&m_statesMutex);

    auto state = injector().inject<TState>();
    assert_return(state.valid(), false);

    pushAndPrepareState(state);

    return true;
  }

private:
  QRecursiveMutex m_statesMutex;

  using StateList = std::vector<StateData>;
  StateList m_states;

  AbstractGameState::SimulationData m_simData;

  osg::ref_ptr<libQtGame::GameUpdateCallback> m_updateCallback;

  std::unique_ptr<GameStatesObject> m_obj;

  void updateStates(const osgHelper::SimulationCallback::SimulationData& data);
  void pushAndPrepareState(const osg::ref_ptr<AbstractGameState>& state);
  void exitState(const osg::ref_ptr<AbstractGameState>& state);

  void onNewGameStateRequest(
    const osg::ref_ptr<AbstractGameState>& current,
    AbstractGameState::NewGameStateMode mode,
    const osg::ref_ptr<AbstractGameState>& newState);
  void onExitGameStateRequest(
    const osg::ref_ptr<AbstractGameState>& current,
    AbstractGameState::ExitGameStateMode mode);

  friend class GameStatesObject;

};

}
