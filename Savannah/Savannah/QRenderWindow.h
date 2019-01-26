#pragma once

#include <QWindow>

#include "IGameWindow.h"

//----------------------------------------------------------

class QRenderViewport;
class QOpenGLContext;

//----------------------------------------------------------

class QRenderWindow final : public QWindow, public IGameWindow
{
	Q_OBJECT
public:
	using Super = QWindow;

	QRenderWindow();
	virtual ~QRenderWindow();

	void				SetRenderViewport(QRenderViewport *viewport);

	// Game Thread
	virtual void		SwapBuffers() override;
	virtual void		Initialize() override;
	virtual void		SwapRenderData(SRenderWindowData *&outRenderData) override;
	virtual void		MakeCurrent() override;
		
	// UI Thread
	void				SetViewportSize(float x, float y);

private:
	QRenderViewport		*m_Viewport = nullptr;
	QOpenGLContext		*m_Context = nullptr;
	QSurfaceFormat		*m_SurfaceFormat = nullptr;
	SRenderWindowData	*m_RenderWindowData_UIThread = nullptr;
};

//----------------------------------------------------------
