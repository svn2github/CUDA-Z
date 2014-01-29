@rem	\file run_studio.bat
@rem	\brief Runs qmake and starts Visual Studio.
@rem	\author Andriy Golovnya <andriy.golovnya@gmail.com> http://ag.embedded.org.ru/
@rem	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
@rem	\license GPLv3 http://www.gnu.org/licenses/gpl-3.0.html

@echo Starting Visual Studio...
@qmake && start cuda-z.sln
