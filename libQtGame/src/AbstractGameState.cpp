#include <libQtGame/AbstractGameState.h>

#include <QtUtilsLib/Multithreading.h>

namespace libQtGame
{

AbstractGameState::AbstractGameState(osgHelper::ioc::Injector& injector)
  : QObject()
  , osg::Referenced()
  , m_injector(&injector)
{
}

AbstractGameState::~AbstractGameState() = default;

void AbstractGameState::onInitialize(const SimulationData& data)
{
}

void AbstractGameState::onUpdate(const SimulationData& data)
{
}

void AbstractGameState::onExit()
{
}

void AbstractGameState::requestExitEventState(ExitGameStateMode mode)
{
  QtUtilsLib::Multithreading::executeInUiAsync([this, mode]()
  {
    Q_EMIT forwardExitEventStateRequest(this, mode);
  });
}

void AbstractGameState::requestResetTimeDelta()
{
  QtUtilsLib::Multithreading::executeInUiAsync([this]()
  {
    Q_EMIT forwardResetTimeDeltaRequest();
  });
}

}
