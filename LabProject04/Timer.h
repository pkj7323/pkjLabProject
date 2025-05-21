#pragma once
constexpr ULONG MAX_SAMPLE_COUNT = 50;
class CGameTimer
{
public:
	CGameTimer();
	virtual ~CGameTimer();

	void Stop() {}
	void Start() {}
	void Reset();
	void Tick(float fLockFPS = 0.0f);	//타이머 갱신
	ULONG GetFrameRate(LPTSTR lpszString = nullptr, int nCharacters = 0);	//프레임레이트 반환
	float GetTimeElapsed();//프레임의 평균 경과시간 반환
private:
	bool			m_bHardwareHasPerformanceCounter;	//컴퓨터가 Performance Counter를 가지고 있는 가
	float			m_fTimeScale;						//Scale Counter의 양
	float			m_fTimeElapsed;						//마지막 프레임 이후 지나간 시간
	__int64			m_nCurrentTime;						//현재의 시간
	__int64			m_nLastTime;						//마지막 프레임의 시간
	__int64			m_nPerformanceFrequency;			//Performance Counter의 주파수

	float			m_fFrameTime[MAX_SAMPLE_COUNT];		//프레임 시간을 누적하기 위한 배열
	ULONG			m_nSampleCount;						//누적된 프레임 횟수

	unsigned long	m_nCurrentFrameRate;				//현재 프레임 레이트
	unsigned long	m_nFramesPerSecond;					//초당 프레임 수
	float			m_fFPSTimeElapsed;					//초당 프레임 수를 계산하기 위한 시간

	bool			m_bStopped;

};

