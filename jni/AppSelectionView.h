/************************************************************************************

Filename    :   AppSelectionView.h
Content     :
Created     :	6/19/2014
Authors     :   Jim Dos�

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#if !defined( AppSelectionView_h )
#define AppSelectionView_h

#include "Kernel/OVR_Array.h"
#include "Lerp.h"
#include "SelectionView.h"
#include "CarouselBrowserComponent.h"
#include "AppManager.h"
#include "UI/UITexture.h"
#include "UI/UIMenu.h"
#include "UI/UIContainer.h"
#include "UI/UILabel.h"
#include "UI/UIImage.h"
#include "UI/UIButton.h"
#include "UI/UITextButton.h"
#include "Settings.h"

using namespace OVR;

namespace VRMatterStreamTheater {

class CinemaApp;
class CarouselBrowserComponent;
class MovieSelectionComponent;

class AppSelectionView : public SelectionView
{
public:
										AppSelectionView( CinemaApp &cinema );
	virtual 							~AppSelectionView();

	virtual void 						OneTimeInit( const char * launchIntent );
	virtual void						OneTimeShutdown();

	virtual void 						OnOpen();
	virtual void 						OnClose();

	virtual Matrix4f 					DrawEyeView( const int eye, const float fovDegrees );
	virtual Matrix4f 					Frame( const VrFrame & vrFrame );
	virtual bool						Command( const char * msg );
	virtual bool 						OnKeyEvent( const int keyCode, const int repeatCount, const KeyEventType eventType );

    void 								SetAppList( const Array<const PcDef *> &movies, const PcDef *nextMovie );
    void								PairSuccess();

	virtual void 						Select( void );
	virtual void 						SelectionHighlighted( bool isHighlighted );
    virtual void 						SetCategory( const PcCategory category );
	virtual void						SetError( const char *text, bool showSDCard, bool showErrorIcon );
	virtual void						ClearError();

private:
	class AppCategoryButton
	{
	public:
		PcCategory		Category;
		String			Text;
		UILabel *		Button;
		float			Width;
		float			Height;

					AppCategoryButton( const PcCategory category, const String &text ) :
						Category( category ), Text( text ), Button( NULL ), Width( 0.0f ), Height( 0.0f ) {}
	};

private:
	CinemaApp &							Cinema;

	UITexture 							SelectionTexture;
	UITexture							Is3DIconTexture;
	UITexture							ShadowTexture;
	UITexture							BorderTexture;
	UITexture							SwipeIconLeftTexture;
	UITexture							SwipeIconRightTexture;
	UITexture							ResumeIconTexture;
	UITexture							ErrorIconTexture;
	UITexture							SDCardTexture;
	UITexture							CloseIconTexture;
	UITexture							SettingsIconTexture;

	UIMenu *							Menu;

	UIContainer *						CenterRoot;

	UILabel * 							ErrorMessage;
	UILabel * 							SDCardMessage;
	UILabel * 							PlainErrorMessage;
	
	bool								ErrorMessageClicked;

	UIContainer *						MovieRoot;
	UIContainer *						CategoryRoot;
	UIContainer *						TitleRoot;

	UILabel	*							MovieTitle;

	UIImage *							SelectionFrame;

	UIImage *							CenterPoster;
	UPInt								CenterIndex;
	Vector3f							CenterPosition;

	UIImage *							LeftSwipes[ 3 ];
	UIImage * 							RightSwipes[ 3 ];

	UILabel	*							ResumeIcon;
	UIButton *							CloseAppButton;
	UIButton *							SettingsButton;

	UILabel *							TimerIcon;
	UILabel *							TimerText;
	double								TimerStartTime;
	int									TimerValue;
	bool								ShowTimer;

	UILabel *							MoveScreenLabel;
	Lerp								MoveScreenAlpha;

	Lerp								SelectionFader;

	CarouselBrowserComponent *			MovieBrowser;
	Array<CarouselItem *> 				MovieBrowserItems;
	Array<PanelPose>					MoviePanelPositions;

	Array<CarouselItemComponent *>	 	MoviePosterComponents;

	Array<AppCategoryButton>			Categories;
    PcCategory			 				CurrentCategory;
	
	Array<const PcDef *> 				AppList;
	int									MoviesIndex;

	const PcDef *						LastMovieDisplayed;

	bool								RepositionScreen;
	bool								HadSelection;

	UIContainer *						settingsMenu;
	UITexture							bgTintTexture;
	UIImage								newPCbg;

	UITextButton *						ButtonGaze;
	UITextButton *						ButtonTrackpad;
	UITextButton *						ButtonGamepad;
	UITextButton *						ButtonOff;
	UITextButton *						Button1080;
	UITextButton *						Button720;
	UITextButton *						Button480;
	UITextButton *						Button60FPS;
	UITextButton *						Button30FPS;
	UITextButton *						ButtonHostAudio;
	UITextButton *						ButtonSaveApp;
	UITextButton *						ButtonSaveDefault;

	int									mouseMode;
	int									streamWidth;
	int									streamHeight;
	int									streamFPS;
	bool								streamHostAudio;

	float								settingsVersion;
	String								defaultSettingsPath;
	String								appSettingsPath;
	Settings*							defaultSettings;
	Settings*							appSettings;



private:
	const PcDef *						GetSelectedApp() const;

	void								TextButtonHelper(UITextButton* button, float scale = 1.0f, int w = 300, int h = 120);
	void 								CreateMenu( OvrGuiSys & guiSys );
	Vector3f 							ScalePosition( const Vector3f &startPos, const float scale, const float menuOffset ) const;
	void 								UpdateMenuPosition();

	void								StartTimer();

	void								UpdateAppTitle();
	void								UpdateSelectionFrame( const VrFrame & vrFrame );

	friend void							AppCloseAppButtonCallback( UIButton *button, void *object );
	void								CloseAppButtonPressed();

	friend void							SettingsButtonCallback( UIButton *button, void *object );
	void								SettingsButtonPressed();

	friend void							SettingsCallback( UITextButton *button, void *object );
	void								SettingsPressed( UITextButton *button );

	friend bool							SettingsSelectedCallback( UITextButton *button, void *object );
	bool								SettingsIsSelected( UITextButton *button );

	friend bool							SettingsActiveCallback( UITextButton *button, void *object );
	bool								SettingsIsActive( UITextButton *button );

	bool								BackPressed();

	bool								ErrorShown() const;
};

} // namespace VRMatterStreamTheater

#endif // AppSelectionView_h
