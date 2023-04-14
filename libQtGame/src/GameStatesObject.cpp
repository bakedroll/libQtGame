#include "GameStatesObject.h"

#include <libQtGame/GameStatesApplication.h>

namespace libQtGame
{

GameStatesObject::GameStatesObject(GameStatesApplication& app, QObject* parent) :
  QObject(parent),
  m_app(app)
{
}

GameStatesObject::~GameStatesObject() = default;

void GameStatesObject::onNewGameStateRequest(
  const osg::ref_ptr<AbstractGameState>& current,
  AbstractGameState::NewGameStateMode mode,
  const osg::ref_ptr<AbstractGameState>& newState)
{
  if (mode == AbstractGameState::NewGameStateMode::ExitCurrent)
  {
    m_app.exitState(current);
  }

  m_app.pushAndPrepareState(newState);
}

void GameStatesObject::onExitGameStateRequest(
  const osg::ref_ptr<AbstractGameState>& current,
  AbstractGameState::ExitGameStateMode mode)
{
  if (mode == AbstractGameState::ExitGameStateMode::ExitCurrent)
  {
    m_app.exitState(current);
    return;
  }

  auto it = m_app.m_states.begin();
  while (it != m_app.m_states.end())
  {
    m_app.exitState(it->state);
    it = m_app.m_states.begin();
  }
}

}
