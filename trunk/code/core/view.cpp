
// tpbds -- wrapper for 3D view/projection

#include "main.h"

View::View(Renderer &renderer) :
	m_renderer(renderer),
	m_camPos(0.f, 0.f, -4.f),
	m_lookAt(0.f),
	m_yFieldOfViewRad(kPI / 4.f),
	m_aspectRatio(renderer.GetAspectRatio()),
	m_zNear(0.1f),
	m_zFar(100.f)
{
	Update();
}

void View::Update()
{
	m_mView = Matrix4x4::View(m_camPos, m_lookAt, Vector3(0.f, 1.f, 0.f));
	m_mProj = Matrix4x4::PerspectiveProjection(m_yFieldOfViewRad, m_aspectRatio, m_zNear, m_zFar);
}

const Matrix4x4 View::UploadMatrices(bool alsoUploadView, const Matrix4x4 *pWorld /* = 0 */) const
{
	if (alsoUploadView)
	{
		m_renderer.SetVertexShaderConstantM4x4(VS_MVIEW_CI, m_mView);
//		m_renderer.SetVertexShaderConstantM4x4(VS_MPROJ_CI, m_mProj);
	}

	const Matrix4x4 mFull = (pWorld) ? m_mProj * m_mView * *pWorld : m_mProj * m_mView;
	m_renderer.SetVertexShaderConstantM4x4(VS_MFULL_CI, mFull);

	return mFull;
}
