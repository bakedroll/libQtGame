#pragma once

#include <QObject>

#include <libQtGame/AbstractGameState.h>

namespace libQtGame
{

class GameStatesApplication;

class GameStatesObject : public QObject
{
  Q_OBJECT

public:
  GameStatesObject(GameStatesApplication& app, QObject* parent = nullptr);
  ~GameStatesObject() override;

public Q_SLOTS:
  void onNewGameStateRequest(const osg::ref_ptr<AbstractGameState>& current,
    AbstractGameState::NewGameStateMode mode,
    const osg::ref_ptr<AbstractGameState>& newState);
  void onExitGameStateRequest(const osg::ref_ptr<AbstractGameState>& current,
    AbstractGameState::ExitGameStateMode mode);

private:
  GameStatesApplication& m_app;

};

}