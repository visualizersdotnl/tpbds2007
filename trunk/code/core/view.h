
// tpbds -- wrapper for 3D view/projection

#ifndef _VIEW_H_
#define _VIEW_H_

class View
{
public:
	View(Renderer &renderer);
	~View() {}

	void Update();	
	const Matrix4x4 UploadMatrices(bool alsoUploadView, const Matrix4x4 *pWorld = 0) const;
	// ^ Returns full World->View->Proj matrix for external use, if needed.

	Renderer &m_renderer;
	
	Vector3 m_camPos;
	Vector3 m_lookAt;
	float m_yFieldOfViewRad;
	float m_aspectRatio;
	float m_zNear, m_zFar;	

	Matrix4x4 m_mView, m_mProj;
};

#endif // _VIEW_H_
