#include <libQtGame/GameStatesApplication.h>

#include <utilsLib/StdOutLoggingStrategy.h>
#include <utilsLib/FileLoggingStrategy.h>
#include <utilsLib/Utils.h>

#include <osgHelper/ResourceManager.h>
#include <osgHelper/ShaderFactory.h>
#include <osgHelper/TextureFactory.h>

namespace libQtGame
{
GameStatesApplication::GameStatesApplication(int& argc, char** argv) :
  QtUtilsApplication<osg::ref_ptr<osg::Referenced>>(argc, argv),
  GameApplication()
{
  utilsLib::ILoggingManager::getLogger()->addLoggingStrategy(
    std::make_shared<utilsLib::StdOutLoggingStrategy>());
  utilsLib::ILoggingManager::getLogger()->addLoggingStrategy(
    std::make_shared<utilsLib::FileLoggingStrategy>("./Logs"));

  setlocale(LC_NUMERIC, "en_US");
}

GameStatesApplication::~GameStatesApplication() = default;

bool GameStatesApplication::notify(QObject* receiver, QEvent* event)
{
  if (safeExecute([&]() { MultithreadedApplication::notify(receiver, event); return 0; }))
  {
    return true;
  }

  return false;
}

int GameStatesApplication::runGame()
{
  return safeExecute([this]()
  {
    m_updateCallback = new libQtGame::GameUpdateCallback(
      std::bind(&GameStatesApplication::updateStates, this, std::placeholders::_1));

    onInitialize(m_updateCallback);

    UTILS_LOG_INFO("Starting mainloop");
    const auto ret = exec();

    // shutdown/free all pointers
    for (auto& state : m_states)
    {
      state.state->onExit();
    }

    m_states.clear();

    onShutdown();

    return ret;
  });
}

void GameStatesApplication::prepareGameState(StateData& data)
{
  data.connections.push_back(connect(data.state.get(), &AbstractGameState::forwardNewEventStateRequest, this,
    &GameStatesApplication::onNewGameStateRequest));
  data.connections.push_back(connect(data.state.get(), &AbstractGameState::forwardExitEventStateRequest, this,
    &GameStatesApplication::onExitGameStateRequest));

  data.connections.push_back(connect(data.state.get(), &AbstractGameState::forwardResetTimeDeltaRequest, [this]()
  {
    m_updateCallback->resetTimeDelta();
  }));

  data.state->onInitialize(m_simData);
  onPrepareGameState(data.state, m_simData);
}

void GameStatesApplication::onException(const std::string& message)
{
  UTILS_LOG_FATAL("A critical exception occured: " + message);
  quit();
}

void GameStatesApplication::registerEssentialComponents(osgHelper::ioc::InjectionContainer& container)
{
  container.registerSingletonInterfaceType<osgHelper::IShaderFactory, osgHelper::ShaderFactory>();
  container.registerSingletonInterfaceType<osgHelper::IResourceManager, osgHelper::ResourceManager>();
  container.registerSingletonInterfaceType<osgHelper::ITextureFactory, osgHelper::TextureFactory>();
}

void GameStatesApplication::updateStates(const osgHelper::SimulationCallback::SimulationData& data)
{
  m_simData = data;

  if (m_states.empty())
  {
    onEmptyStateList();
  }

  for (auto& state : m_states)
  {
    state.state->onUpdate(data);
  }
}

void GameStatesApplication::pushAndPrepareState(const osg::ref_ptr<AbstractGameState>& state)
{
  StateData data;
  data.state = state;

  m_states.push_back(data);
  prepareGameState(data);
}

void GameStatesApplication::exitState(const osg::ref_ptr<AbstractGameState>& state)
{
  for (auto it = m_states.begin(); it != m_states.end(); ++it)
  {
    if (it->state == state)
    {
      onExitGameState(state);
      state->onExit();

      m_states.erase(it);
      return;
    }
  }

  UTILS_LOG_FATAL("Attempting to exit unknown state");
}

void GameStatesApplication::onNewGameStateRequest(const osg::ref_ptr<AbstractGameState>& current,
  AbstractGameState::NewGameStateMode mode, const osg::ref_ptr<AbstractGameState>& newState)
{
  if (mode == AbstractGameState::NewGameStateMode::ExitCurrent)
  {
    exitState(current);
  }

  pushAndPrepareState(newState);
}

void GameStatesApplication::onExitGameStateRequest(const osg::ref_ptr<AbstractGameState>& current,
  AbstractGameState::ExitGameStateMode mode)
{
  if (mode == AbstractGameState::ExitGameStateMode::ExitCurrent)
  {
    exitState(current);
    return;
  }

  auto it = m_states.begin();
  while (it != m_states.end())
  {
    exitState(it->state);
    it = m_states.begin();
  }
}

}
