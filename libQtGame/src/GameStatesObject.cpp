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
  m_app.onNewGameStateRequest(current, mode, newState);
}

void GameStatesObject::onExitGameStateRequest(
  const osg::ref_ptr<AbstractGameState>& current,
  AbstractGameState::ExitGameStateMode mode)
{
  m_app.onExitGameStateRequest(current, mode);
}

}
