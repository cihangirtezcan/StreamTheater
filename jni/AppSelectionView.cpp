/************************************************************************************

Filename    :   AppSelectionView.cpp
Content     :
Created     :	6/19/2014
Authors     :   Jim Dos�

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include <unistd.h>

#include <android/keycodes.h>
#include "Kernel/OVR_String_Utils.h"
#include "App.h"
#include "AppSelectionView.h"
#include "CinemaApp.h"
#include "VRMenu/VRMenuMgr.h"
#include "VRMenu/GuiSys.h"
#include "PcCategoryComponent.h"
#include "MoviePosterComponent.h"
#include "MovieSelectionComponent.h"
#include "SwipeHintComponent.h"
#include "PackageFiles.h"
#include "CinemaStrings.h"
#include "BitmapFont.h"
#include "Native.h"

namespace VRMatterStreamTheater {

static const int PosterWidth = 228;
static const int PosterHeight = 344;

static const Vector3f PosterScale( 4.4859375f * 0.98f );

static const double TimerTotalTime = 10;

static const int NumSwipeTrails = 3;

//TODO: Instead of setting its own category it really needs to adopt it from PCSelView, but it doesn't know about that yet
//=======================================================================================

AppSelectionView::AppSelectionView( CinemaApp &cinema ) :
	SelectionView( "AppSelectionView" ),
	Cinema( cinema ),
	SelectionTexture(),
	Is3DIconTexture(),
	ShadowTexture(),
	BorderTexture(),
	SwipeIconLeftTexture(),
	SwipeIconRightTexture(),
	ResumeIconTexture(),
	ErrorIconTexture(),
	SDCardTexture(),
	CloseIconTexture(),
	SettingsIconTexture(),
	Menu( NULL ),
	CenterRoot( NULL ),
	ErrorMessage( NULL ),
	SDCardMessage( NULL ),
	PlainErrorMessage( NULL ),
	ErrorMessageClicked( false ),
	MovieRoot( NULL ),
	CategoryRoot( NULL ),
	TitleRoot( NULL ),
	MovieTitle( NULL ),
	SelectionFrame( NULL ),
	CenterPoster( NULL ),
	CenterIndex( 0 ),
	CenterPosition(),
	LeftSwipes(),
	RightSwipes(),
	ResumeIcon( NULL ),
	CloseAppButton( NULL ),
	SettingsButton( NULL ),
	TimerIcon( NULL ),
	TimerText( NULL ),
	TimerStartTime( 0 ),
	TimerValue( 0 ),
	ShowTimer( false ),
	MoveScreenLabel( NULL ),
	MoveScreenAlpha(),
	SelectionFader(),
	MovieBrowser( NULL ),
	MoviePanelPositions(),
	MoviePosterComponents(),
	Categories(),
	CurrentCategory( CATEGORY_LIMELIGHT ),
	AppList(),
	MoviesIndex( 0 ),
	LastMovieDisplayed( NULL ),
	RepositionScreen( false ),
	HadSelection( false ),
	settingsMenu( NULL ),
	bgTintTexture(),
	newPCbg( Cinema ),
	ButtonGaze( NULL ),
	ButtonTrackpad( NULL ),
	ButtonGamepad( NULL ),
	ButtonOff( NULL ),
	Button1080( NULL ),
	Button720( NULL ),
	Button60FPS( NULL ),
	Button30FPS( NULL ),
	ButtonHostAudio( NULL ),
	ButtonSaveApp( NULL ),
	ButtonSaveDefault( NULL ),
	mouseMode( MOUSE_GAZE ),
	streamWidth( 1280 ),
	streamHeight( 720 ),
	streamFPS( 60 ),
	streamHostAudio( 0 ),
	settingsVersion(1.0f),
	defaultSettingsPath(""),
	appSettingsPath(""),
	defaultSettings( NULL ),
	appSettings( NULL )
{
	// This is called at library load time, so the system is not initialized
	// properly yet.
}

AppSelectionView::~AppSelectionView()
{
	DeletePointerArray( MovieBrowserItems );
}

void AppSelectionView::OneTimeInit( const char * launchIntent )
{
	LOG( "AppSelectionView::OneTimeInit" );

	const double start = vrapi_GetTimeInSeconds();

	CreateMenu( Cinema.GetGuiSys() );

	SetCategory( CATEGORY_LIMELIGHT );

	LOG( "AppSelectionView::OneTimeInit %3.1f seconds", vrapi_GetTimeInSeconds() - start );
}

void AppSelectionView::OneTimeShutdown()
{
	LOG( "AppSelectionView::OneTimeShutdown" );
}

void AppSelectionView::OnOpen()
{
	LOG( "OnOpen" );
	CurViewState = VIEWSTATE_OPEN;

	LastMovieDisplayed = NULL;
	HadSelection = NULL;

	if ( Cinema.InLobby )
	{
		Cinema.SceneMgr.SetSceneModel( *Cinema.ModelMgr.BoxOffice );
		Cinema.SceneMgr.UseOverlay = false;

		Vector3f size( PosterWidth * VRMenuObject::DEFAULT_TEXEL_SCALE * PosterScale.x, PosterHeight * VRMenuObject::DEFAULT_TEXEL_SCALE * PosterScale.y, 0.0f );

		Cinema.SceneMgr.SceneScreenBounds = Bounds3f( size * -0.5f, size * 0.5f );
		Cinema.SceneMgr.SceneScreenBounds.Translate( Vector3f(  0.00f, 1.76f,  -7.25f ) );
	}

	Cinema.SceneMgr.LightsOn( 1.5f );

	const double now = vrapi_GetTimeInSeconds();
	SelectionFader.Set( now, 0, now + 0.1, 1.0f );

	Menu->SetFlags( VRMENU_FLAG_SHORT_PRESS_HANDLED_BY_APP );

	if ( Cinema.InLobby )
	{
		CategoryRoot->SetVisible( true );
	}
	else
	{
		CategoryRoot->SetVisible( false );
		Menu->SetFlags( VRMenuFlags_t() );
	}

	ResumeIcon->SetVisible( false );
	TimerIcon->SetVisible( false );
	CenterRoot->SetVisible( true );
	
	MoveScreenLabel->SetVisible( false );

	MovieBrowser->SetSelectionIndex( MoviesIndex );

	RepositionScreen = false;

	UpdateMenuPosition();
	Cinema.SceneMgr.ClearGazeCursorGhosts();
	Menu->Open();

	MoviePosterComponent::ShowShadows = Cinema.InLobby;

	SwipeHintComponent::ShowSwipeHints = true;
	
	const PcDef* selectedPC = Cinema.GetCurrentPc();
	String uuid = selectedPC->UUID;
	Native::PairState ps = Native::GetPairState(Cinema.app, uuid.ToCStr());
	if (ps == Native::PAIRED) {
		LOG( "Paired");
		Native::InitAppSelector(Cinema.app, uuid.ToCStr());
	} else {
		LOG( "Not Paired!");
		Native::Pair(Cinema.app, uuid.ToCStr());
	}
}

void AppSelectionView::PairSuccess() {
	const PcDef* selectedPC = Cinema.GetCurrentPc();
	String uuid = selectedPC->UUID;

	Cinema.AppSelection(true);
}

void AppSelectionView::OnClose()
{
	LOG( "OnClose" );
	ShowTimer = false;
	CurViewState = VIEWSTATE_CLOSED;
	CenterRoot->SetVisible( false );
	Menu->Close();
	Cinema.SceneMgr.ClearMovie();
}

bool AppSelectionView::Command( const char * msg )
{
	return false;
}

bool AppSelectionView::BackPressed()
{
	if(ErrorShown())
	{
		ClearError();
		return true;
	}
	if(settingsMenu->GetVisible())
	{
		settingsMenu->SetVisible(false);

		return true;
	}

	Cinema.PcSelection(true);
	return true;
}

bool AppSelectionView::OnKeyEvent( const int keyCode, const int repeatCount, const KeyEventType eventType )
{
	switch ( keyCode )
	{
		case AKEYCODE_BACK:
		{
			switch ( eventType )
			{
				case KEY_EVENT_DOUBLE_TAP:
					LOG( "KEY_EVENT_DOUBLE_TAP" );
					return true;
					break;
				case KEY_EVENT_SHORT_PRESS:
					BackPressed();
					return true;
					break;
				default:
					break;
			}
		}
	}
	return false;
}

//=======================================================================================

void AppCloseAppButtonCallback( UIButton *button, void *object )
{
	( ( AppSelectionView * )object )->CloseAppButtonPressed();
}

void SettingsButtonCallback( UIButton *button, void *object )
{
	( ( AppSelectionView * )object )->SettingsButtonPressed();
}

void SettingsCallback( UITextButton *button, void *object )
{
	( ( AppSelectionView * )object )->SettingsPressed(button);
}

bool SettingsSelectedCallback( UITextButton *button, void *object )
{
	return ( ( AppSelectionView * )object )->SettingsIsSelected(button);
}

bool SettingsActiveCallback( UITextButton *button, void *object )
{
	return ( ( AppSelectionView * )object )->SettingsIsActive(button);
}



void AppSelectionView::TextButtonHelper(UITextButton* button, float scale, int w, int h)
{
	button->SetLocalScale( Vector3f( scale ) );
	button->SetFontScale( 1.0f );
	button->SetColor( Vector4f( 0.0f, 0.0f, 0.0f, 1.0f ) );
	button->SetTextColor( Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ) );
	button->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, bgTintTexture, w, h );
}

void AppSelectionView::CreateMenu( OvrGuiSys & guiSys )
{
	const Quatf forward( Vector3f( 0.0f, 1.0f, 0.0f ), 0.0f );

	// ==============================================================================
	//
	// load textures
	//
	SelectionTexture.LoadTextureFromApplicationPackage( "assets/selection.png" );
	Is3DIconTexture.LoadTextureFromApplicationPackage( "assets/3D_icon.png" );
	ShadowTexture.LoadTextureFromApplicationPackage( "assets/shadow.png" );
	BorderTexture.LoadTextureFromApplicationPackage( "assets/category_border.png" );
	SwipeIconLeftTexture.LoadTextureFromApplicationPackage( "assets/SwipeSuggestionArrowLeft.png" );
	SwipeIconRightTexture.LoadTextureFromApplicationPackage( "assets/SwipeSuggestionArrowRight.png" );
	ResumeIconTexture.LoadTextureFromApplicationPackage( "assets/resume.png" );
	ErrorIconTexture.LoadTextureFromApplicationPackage( "assets/error.png" );
	SDCardTexture.LoadTextureFromApplicationPackage( "assets/sdcard.png" );
	CloseIconTexture.LoadTextureFromApplicationPackage( "assets/close.png" );
	SettingsIconTexture.LoadTextureFromApplicationPackage( "assets/settings.png" );

	bgTintTexture.LoadTextureFromApplicationPackage( "assets/backgroundTint.png" );

 	// ==============================================================================
	//
	// create menu
	//
	Menu = new UIMenu( Cinema );
	Menu->Create( "AppBrowser" );

	CenterRoot = new UIContainer( Cinema );
	CenterRoot->AddToMenu( guiSys, Menu );
	CenterRoot->SetLocalPose( forward, Vector3f( 0.0f, 0.0f, 0.0f ) );

	MovieRoot = new UIContainer( Cinema );
	MovieRoot->AddToMenu( guiSys, Menu, CenterRoot );
	MovieRoot->SetLocalPose( forward, Vector3f( 0.0f, 0.0f, 0.0f ) );

	CategoryRoot = new UIContainer( Cinema );
	CategoryRoot->AddToMenu( guiSys, Menu, CenterRoot );
	CategoryRoot->SetLocalPose( forward, Vector3f( 0.0f, 0.0f, 0.0f ) );

	TitleRoot = new UIContainer( Cinema );
	TitleRoot->AddToMenu( guiSys, Menu, CenterRoot );
	TitleRoot->SetLocalPose( forward, Vector3f( 0.0f, 0.0f, 0.0f ) );

	// ==============================================================================
	//
	// error message
	//
	ErrorMessage = new UILabel( Cinema );
	ErrorMessage->AddToMenu( guiSys, Menu, CenterRoot );
	ErrorMessage->SetLocalPose( forward, Vector3f( 0.00f, 1.76f, -7.39f + 0.5f ) );
	ErrorMessage->SetLocalScale( Vector3f( 5.0f ) );
	ErrorMessage->SetFontScale( 0.5f );
	ErrorMessage->SetTextOffset( Vector3f( 0.0f, -48 * VRMenuObject::DEFAULT_TEXEL_SCALE, 0.0f ) );
	ErrorMessage->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, ErrorIconTexture );
	ErrorMessage->SetVisible( false );

	// ==============================================================================
	//
	// sdcard icon
	//
	SDCardMessage = new UILabel( Cinema );
	SDCardMessage->AddToMenu( guiSys, Menu, CenterRoot );
	SDCardMessage->SetLocalPose( forward, Vector3f( 0.00f, 1.76f + 330.0f * VRMenuObject::DEFAULT_TEXEL_SCALE, -7.39f + 0.5f ) );
	SDCardMessage->SetLocalScale( Vector3f( 5.0f ) );
	SDCardMessage->SetFontScale( 0.5f );
	SDCardMessage->SetTextOffset( Vector3f( 0.0f, -96 * VRMenuObject::DEFAULT_TEXEL_SCALE, 0.0f ) );
	SDCardMessage->SetVisible( false );
	SDCardMessage->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, SDCardTexture );

	// ==============================================================================
	//
	// error without icon
	//
	PlainErrorMessage = new UILabel( Cinema );
	PlainErrorMessage->AddToMenu( guiSys, Menu, CenterRoot );
	PlainErrorMessage->SetLocalPose( forward, Vector3f( 0.00f, 1.76f + ( 330.0f - 48 ) * VRMenuObject::DEFAULT_TEXEL_SCALE, -7.39f + 0.5f ) );
	PlainErrorMessage->SetLocalScale( Vector3f( 5.0f ) );
	PlainErrorMessage->SetFontScale( 0.5f );
	PlainErrorMessage->SetVisible( false );

	// ==============================================================================
	//
	// movie browser
	//
	MoviePanelPositions.PushBack( PanelPose( forward, Vector3f( -5.59f, 1.76f, -12.55f ), Vector4f( 0.0f, 0.0f, 0.0f, 0.0f ) ) );
	MoviePanelPositions.PushBack( PanelPose( forward, Vector3f( -3.82f, 1.76f, -10.97f ), Vector4f( 0.1f, 0.1f, 0.1f, 1.0f ) ) );
	MoviePanelPositions.PushBack( PanelPose( forward, Vector3f( -2.05f, 1.76f,  -9.39f ), Vector4f( 0.2f, 0.2f, 0.2f, 1.0f ) ) );
	MoviePanelPositions.PushBack( PanelPose( forward, Vector3f(  0.00f, 1.76f,  -7.39f ), Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ) ) );
	MoviePanelPositions.PushBack( PanelPose( forward, Vector3f(  2.05f, 1.76f,  -9.39f ), Vector4f( 0.2f, 0.2f, 0.2f, 1.0f ) ) );
	MoviePanelPositions.PushBack( PanelPose( forward, Vector3f(  3.82f, 1.76f, -10.97f ), Vector4f( 0.1f, 0.1f, 0.1f, 1.0f ) ) );
	MoviePanelPositions.PushBack( PanelPose( forward, Vector3f(  5.59f, 1.76f, -12.55f ), Vector4f( 0.0f, 0.0f, 0.0f, 0.0f ) ) );

	CenterIndex = MoviePanelPositions.GetSize() / 2;
	CenterPosition = MoviePanelPositions[ CenterIndex ].Position;

	MovieBrowser = new CarouselBrowserComponent( MovieBrowserItems, MoviePanelPositions );
	MovieRoot->AddComponent( MovieBrowser );

	// ==============================================================================
	//
	// selection rectangle
	//
	SelectionFrame = new UIImage( Cinema );
	SelectionFrame->AddToMenu( guiSys, Menu, MovieRoot );
	SelectionFrame->SetLocalPose( forward, CenterPosition + Vector3f( 0.0f, 0.0f, 0.1f ) );
	SelectionFrame->SetLocalScale( PosterScale );
	SelectionFrame->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, SelectionTexture );
	SelectionFrame->AddComponent( new MovieSelectionComponent( this ) );

	const Vector3f selectionBoundsExpandMin = Vector3f( 0.0f );
	const Vector3f selectionBoundsExpandMax = Vector3f( 0.0f, -0.13f, 0.0f );
	SelectionFrame->GetMenuObject()->SetLocalBoundsExpand( selectionBoundsExpandMin, selectionBoundsExpandMax );

	// ==============================================================================
	//
	// add shadow and 3D icon to movie poster panels
	//
	Array<VRMenuObject *> menuObjs;
	for ( UPInt i = 0; i < MoviePanelPositions.GetSize(); ++i )
	{
		UIContainer *posterContainer = new UIContainer( Cinema );
		posterContainer->AddToMenu( guiSys, Menu, MovieRoot );
		posterContainer->SetLocalPose( MoviePanelPositions[ i ].Orientation, MoviePanelPositions[ i ].Position );
		posterContainer->GetMenuObject()->AddFlags( VRMENUOBJECT_FLAG_NO_FOCUS_GAINED );

		//
		// posters
		//
		UIImage * posterImage = new UIImage( Cinema );
		posterImage->AddToMenu( guiSys, Menu, posterContainer );
		posterImage->SetLocalPose( forward, Vector3f( 0.0f, 0.0f, 0.0f ) );
		posterImage->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, SelectionTexture.Texture, PosterWidth, PosterHeight );
		posterImage->SetLocalScale( PosterScale );
		posterImage->GetMenuObject()->AddFlags( VRMENUOBJECT_FLAG_NO_FOCUS_GAINED );
		posterImage->GetMenuObject()->SetLocalBoundsExpand( selectionBoundsExpandMin, selectionBoundsExpandMax );

		if ( i == CenterIndex )
		{
			CenterPoster = posterImage;
		}

		//
		// 3D icon
		//
		UIImage * is3DIcon = new UIImage( Cinema );
		is3DIcon->AddToMenu( guiSys, Menu, posterContainer );
		is3DIcon->SetLocalPose( forward, Vector3f( 0.75f, 1.3f, 0.02f ) );
		is3DIcon->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, Is3DIconTexture );
		is3DIcon->SetLocalScale( PosterScale );
		is3DIcon->GetMenuObject()->AddFlags( VRMenuObjectFlags_t( VRMENUOBJECT_FLAG_NO_FOCUS_GAINED ) | VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ) );

		//
		// shadow
		//
		UIImage * shadow = new UIImage( Cinema );
		shadow->AddToMenu( guiSys, Menu, posterContainer );
		shadow->SetLocalPose( forward, Vector3f( 0.0f, -1.97f, 0.00f ) );
		shadow->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, ShadowTexture );
		shadow->SetLocalScale( PosterScale );
		shadow->GetMenuObject()->AddFlags( VRMenuObjectFlags_t( VRMENUOBJECT_FLAG_NO_FOCUS_GAINED ) | VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ) );

		//
		// add the component
		//
		MoviePosterComponent *posterComp = new MoviePosterComponent();
		posterComp->SetMenuObjects( PosterWidth, PosterHeight, posterContainer, posterImage, is3DIcon, shadow );
		posterContainer->AddComponent( posterComp );

		menuObjs.PushBack( posterContainer->GetMenuObject() );
		MoviePosterComponents.PushBack( posterComp );
	}

	MovieBrowser->SetMenuObjects( menuObjs, MoviePosterComponents );

	// ==============================================================================
	//
	// category browser
	//
	Categories.PushBack( AppCategoryButton( CATEGORY_LIMELIGHT, CinemaStrings::Category_LimeLight ) );

	// create the buttons and calculate their size
	const float itemWidth = 1.10f;
	float categoryBarWidth = 0.0f;
	for ( UPInt i = 0; i < Categories.GetSize(); ++i )
	{
		Categories[ i ].Button = new UILabel( Cinema );
		Categories[ i ].Button->AddToMenu( guiSys, Menu, CategoryRoot );
		Categories[ i ].Button->SetFontScale( 2.2f );
		Categories[ i ].Button->SetText( Categories[ i ].Text.ToCStr() );
		Categories[ i ].Button->AddComponent( new PcCategoryComponent( this, Categories[ i ].Category ) );

		const Bounds3f & bounds = Categories[ i ].Button->GetTextLocalBounds( Cinema.GetGuiSys().GetDefaultFont() );
		Categories[ i ].Width = OVR::Alg::Max( bounds.GetSize().x, itemWidth ) + 80.0f * VRMenuObject::DEFAULT_TEXEL_SCALE;
		Categories[ i ].Height = bounds.GetSize().y + 108.0f * VRMenuObject::DEFAULT_TEXEL_SCALE;
		categoryBarWidth += Categories[ i ].Width;
	}

	// reposition the buttons and set the background and border
	float startX = categoryBarWidth * -0.5f;
	for ( UPInt i = 0; i < Categories.GetSize(); ++i )
	{
		VRMenuSurfaceParms panelSurfParms( "",
				BorderTexture.Texture, BorderTexture.Width, BorderTexture.Height, SURFACE_TEXTURE_ADDITIVE,
				0, 0, 0, SURFACE_TEXTURE_MAX,
				0, 0, 0, SURFACE_TEXTURE_MAX );

		panelSurfParms.Border = Vector4f( 14.0f );
		panelSurfParms.Dims = Vector2f( Categories[ i ].Width * VRMenuObject::TEXELS_PER_METER, Categories[ i ].Height * VRMenuObject::TEXELS_PER_METER );

		Categories[ i ].Button->SetImage( 0, panelSurfParms );
		Categories[ i ].Button->SetLocalPose( forward, Vector3f( startX + Categories[ i ].Width * 0.5f, 3.6f, -7.39f ) );
		Categories[ i ].Button->SetLocalBoundsExpand( Vector3f( 0.0f, 0.13f, 0.0f ), Vector3f::ZERO );

    	startX += Categories[ i ].Width;
	}

	// ==============================================================================
	//
	// movie title
	//
	MovieTitle = new UILabel( Cinema );
	MovieTitle->AddToMenu( guiSys, Menu, TitleRoot );
	MovieTitle->SetLocalPose( forward, Vector3f( 0.0f, 0.045f, -7.37f ) );
	MovieTitle->GetMenuObject()->AddFlags( VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ) );
	MovieTitle->SetFontScale( 2.5f );

	// ==============================================================================
	//
	// swipe icons
	//
	float yPos = 1.76f - ( PosterHeight - SwipeIconLeftTexture.Height ) * 0.5f * VRMenuObject::DEFAULT_TEXEL_SCALE * PosterScale.y;

	for( int i = 0; i < NumSwipeTrails; i++ )
	{
		Vector3f swipeIconPos = Vector3f( 0.0f, yPos, -7.17f + 0.01f * ( float )i );

		LeftSwipes[ i ] = new UIImage( Cinema );
		LeftSwipes[ i ]->AddToMenu( guiSys, Menu, CenterRoot );
		LeftSwipes[ i ]->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, SwipeIconLeftTexture );
		LeftSwipes[ i ]->SetLocalScale( PosterScale );
		LeftSwipes[ i ]->GetMenuObject()->AddFlags( VRMenuObjectFlags_t( VRMENUOBJECT_FLAG_NO_DEPTH ) | VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ) );
		LeftSwipes[ i ]->AddComponent( new SwipeHintComponent( MovieBrowser, false, 1.3333f, 0.4f + ( float )i * 0.13333f, 5.0f ) );

		swipeIconPos.x = ( ( PosterWidth + SwipeIconLeftTexture.Width * ( i + 2 ) ) * -0.5f ) * VRMenuObject::DEFAULT_TEXEL_SCALE * PosterScale.x;
		LeftSwipes[ i ]->SetLocalPosition( swipeIconPos );

		RightSwipes[ i ] = new UIImage( Cinema );
		RightSwipes[ i ]->AddToMenu( guiSys, Menu, CenterRoot );
		RightSwipes[ i ]->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, SwipeIconRightTexture );
		RightSwipes[ i ]->SetLocalScale( PosterScale );
		RightSwipes[ i ]->GetMenuObject()->AddFlags( VRMenuObjectFlags_t( VRMENUOBJECT_FLAG_NO_DEPTH ) | VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ) );
		RightSwipes[ i ]->AddComponent( new SwipeHintComponent( MovieBrowser, true, 1.3333f, 0.4f + ( float )i * 0.13333f, 5.0f ) );

		swipeIconPos.x = ( ( PosterWidth + SwipeIconRightTexture.Width * ( i + 2 ) ) * 0.5f ) * VRMenuObject::DEFAULT_TEXEL_SCALE * PosterScale.x;
		RightSwipes[ i ]->SetLocalPosition( swipeIconPos );
    }

	// ==============================================================================
	//
	// resume icon
	//
	ResumeIcon = new UILabel( Cinema );
	ResumeIcon->AddToMenu( guiSys, Menu, MovieRoot );
	ResumeIcon->SetLocalPose( forward, CenterPosition + Vector3f( 0.0f, 0.0f, 0.5f ) );
	ResumeIcon->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, ResumeIconTexture );
	ResumeIcon->GetMenuObject()->AddFlags( VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ) );
	ResumeIcon->SetFontScale( 0.3f );
	ResumeIcon->SetLocalScale( 6.0f );
	ResumeIcon->SetText( CinemaStrings::MovieSelection_Resume.ToCStr() );
	ResumeIcon->SetTextOffset( Vector3f( 0.0f, -ResumeIconTexture.Height * VRMenuObject::DEFAULT_TEXEL_SCALE * 0.5f, 0.0f ) );
	ResumeIcon->SetVisible( false );

	// ==============================================================================
	//
	// close app button
	//
	CloseAppButton = new UIButton( Cinema );
	CloseAppButton->AddToMenu( guiSys, Menu, MovieRoot );
	CloseAppButton->SetLocalPose( forward, CenterPosition + Vector3f( 0.8f, -1.28f, 0.5f ) );
	CloseAppButton->SetButtonImages( CloseIconTexture, CloseIconTexture, CloseIconTexture );
	CloseAppButton->SetLocalScale( 3.0f );
	CloseAppButton->SetVisible( false );
	CloseAppButton->SetOnClick( AppCloseAppButtonCallback, this );

	// ==============================================================================
	//
	// settings button
	//
	SettingsButton = new UIButton( Cinema );
	SettingsButton->AddToMenu( guiSys, Menu, MovieRoot );
	SettingsButton->SetLocalPose( forward, CenterPosition + Vector3f( -0.8f, -1.28f, 0.5f ) );
	SettingsButton->SetButtonImages( SettingsIconTexture, SettingsIconTexture, SettingsIconTexture );
	SettingsButton->SetLocalScale( 3.0f );
	SettingsButton->SetVisible( true );
	SettingsButton->SetOnClick( SettingsButtonCallback, this );

	// ==============================================================================
	//
	// timer
	//
	TimerIcon = new UILabel( Cinema );
	TimerIcon->AddToMenu( guiSys, Menu, MovieRoot );
	TimerIcon->SetLocalPose( forward, CenterPosition + Vector3f( 0.0f, 0.0f, 0.5f ) );
	TimerIcon->GetMenuObject()->AddFlags( VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ) );
	TimerIcon->SetFontScale( 1.0f );
	TimerIcon->SetLocalScale( 2.0f );
	TimerIcon->SetText( "10" );
	TimerIcon->SetVisible( false );

	VRMenuSurfaceParms timerSurfaceParms( "timer",
		"assets/timer.tga", SURFACE_TEXTURE_DIFFUSE,
		"assets/timer_fill.tga", SURFACE_TEXTURE_COLOR_RAMP_TARGET,
		"assets/color_ramp_timer.tga", SURFACE_TEXTURE_COLOR_RAMP );

	TimerIcon->SetImage( 0, timerSurfaceParms );

	// text
	TimerText = new UILabel( Cinema );
	TimerText->AddToMenu( guiSys, Menu, TimerIcon );
	TimerText->SetLocalPose( forward, Vector3f( 0.0f, -0.3f, 0.0f ) );
	TimerText->GetMenuObject()->AddFlags( VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ) );
	TimerText->SetFontScale( 1.0f );
	TimerText->SetText( CinemaStrings::MovieSelection_Next );

	// ==============================================================================
	//
	// reorient message
	//
	MoveScreenLabel = new UILabel( Cinema );
	MoveScreenLabel->AddToMenu( guiSys, Menu, NULL );
	MoveScreenLabel->SetLocalPose( forward, Vector3f( 0.0f, 0.0f, -1.8f ) );
	MoveScreenLabel->GetMenuObject()->AddFlags( VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ) );
	MoveScreenLabel->SetFontScale( 0.5f );
	MoveScreenLabel->SetText( CinemaStrings::MoviePlayer_Reorient );
	MoveScreenLabel->SetTextOffset( Vector3f( 0.0f, -24 * VRMenuObject::DEFAULT_TEXEL_SCALE, 0.0f ) );  // offset to be below gaze cursor
	MoveScreenLabel->SetVisible( false );

	// ==============================================================================
	//
	// Settings
	//
	settingsMenu = new UIContainer( Cinema );
	settingsMenu->AddToMenu( guiSys, Menu );
	settingsMenu->SetLocalPose( forward, Vector3f( 0.0f, 2.0f, -3.0f ) );
	settingsMenu->SetVisible(false);

	newPCbg.AddToMenu( guiSys, Menu, settingsMenu);
	newPCbg.SetImage( 0, SURFACE_TEXTURE_DIFFUSE, bgTintTexture, 700, 900 );

	static const float column1 = -0.32f;
	static const float column2 = 0.32f;
	static const float rowstart = 0.775f;
	static const float rowinc = -0.25f;
	float rowpos = rowstart;

	Button1080 = new UITextButton( Cinema );
	Button1080->AddToMenu( guiSys, Menu, settingsMenu );
	Button1080->SetLocalPosition( Vector3f( column1, rowpos += rowinc, 0.1f ) );
	Button1080->SetText( CinemaStrings::ButtonText_Button1080 );
	TextButtonHelper(Button1080);
	Button1080->SetOnClick( SettingsCallback, this);
	Button1080->SetIsSelected( SettingsSelectedCallback, this);

	Button720 = new UITextButton( Cinema );
	Button720->AddToMenu( guiSys, Menu, settingsMenu );
	Button720->SetLocalPosition( Vector3f( column1, rowpos += rowinc, 0.1f ) );
	Button720->SetText( CinemaStrings::ButtonText_Button720 );
	TextButtonHelper(Button720);
	Button720->SetOnClick( SettingsCallback, this);
	Button720->SetIsSelected( SettingsSelectedCallback, this);

	Button60FPS = new UITextButton( Cinema );
	Button60FPS->AddToMenu( guiSys, Menu, settingsMenu );
	Button60FPS->SetLocalPosition( Vector3f( column1, rowpos += rowinc, 0.1f ) );
	Button60FPS->SetText( CinemaStrings::ButtonText_Button60FPS );
	TextButtonHelper(Button60FPS);
	Button60FPS->SetOnClick( SettingsCallback, this);
	Button60FPS->SetIsSelected( SettingsSelectedCallback, this);

	Button30FPS = new UITextButton( Cinema );
	Button30FPS->AddToMenu( guiSys, Menu, settingsMenu );
	Button30FPS->SetLocalPosition( Vector3f( column1, rowpos += rowinc, 0.1f ) );
	Button30FPS->SetText( CinemaStrings::ButtonText_Button30FPS );
	TextButtonHelper(Button30FPS);
	Button30FPS->SetOnClick( SettingsCallback, this);
	Button30FPS->SetIsSelected( SettingsSelectedCallback, this);

	// skip 1/4 a space
	rowpos += rowinc /4;

	ButtonHostAudio = new UITextButton( Cinema );
	ButtonHostAudio->AddToMenu( guiSys, Menu, settingsMenu );
	ButtonHostAudio->SetLocalPosition( Vector3f( column1, rowpos += rowinc, 0.1f ) );
	ButtonHostAudio->SetText( CinemaStrings::ButtonText_ButtonHostAudio );
	TextButtonHelper(ButtonHostAudio);
	ButtonHostAudio->SetOnClick( SettingsCallback, this);
	ButtonHostAudio->SetIsSelected( SettingsSelectedCallback, this);

	// restart next column
	rowpos = rowstart;

	ButtonGaze = new UITextButton( Cinema );
	ButtonGaze->AddToMenu( guiSys, Menu, settingsMenu );
	ButtonGaze->SetLocalPosition( Vector3f( column2, rowpos += rowinc, 0.1f ) );
	ButtonGaze->SetText( CinemaStrings::ButtonText_ButtonGaze );
	TextButtonHelper(ButtonGaze);
	ButtonGaze->SetOnClick( SettingsCallback, this);
	ButtonGaze->SetIsSelected( SettingsSelectedCallback, this);

	ButtonTrackpad = new UITextButton( Cinema );
	ButtonTrackpad->AddToMenu( guiSys, Menu, settingsMenu );
	ButtonTrackpad->SetLocalPosition( Vector3f( column2, rowpos += rowinc, 0.1f ) );
	ButtonTrackpad->SetText( CinemaStrings::ButtonText_ButtonTrackpad );
	TextButtonHelper(ButtonTrackpad);
	ButtonTrackpad->SetOnClick( SettingsCallback, this);
	ButtonTrackpad->SetIsSelected( SettingsSelectedCallback, this);

	ButtonGamepad = new UITextButton( Cinema );
	ButtonGamepad->AddToMenu( guiSys, Menu, settingsMenu );
	ButtonGamepad->SetLocalPosition( Vector3f( column2, rowpos += rowinc, 0.1f ) );
	ButtonGamepad->SetText( CinemaStrings::ButtonText_ButtonGamepad );
	TextButtonHelper(ButtonGamepad);
	ButtonGamepad->SetOnClick( SettingsCallback, this);
	ButtonGamepad->SetIsSelected( SettingsSelectedCallback, this);

	ButtonOff = new UITextButton( Cinema );
	ButtonOff->AddToMenu( guiSys, Menu, settingsMenu );
	ButtonOff->SetLocalPosition( Vector3f( column2, rowpos += rowinc, 0.1f ) );
	ButtonOff->SetText( CinemaStrings::ButtonText_ButtonOff );
	TextButtonHelper(ButtonOff);
	ButtonOff->SetOnClick( SettingsCallback, this);
	ButtonOff->SetIsSelected( SettingsSelectedCallback, this);

	// skip half a space
	rowpos += rowinc /2;

	ButtonSaveApp = new UITextButton( Cinema );
	ButtonSaveApp->AddToMenu( guiSys, Menu, settingsMenu );
	ButtonSaveApp->SetLocalPosition( Vector3f( column2, rowpos += rowinc, 0.1f ) );
	ButtonSaveApp->SetText( CinemaStrings::ButtonText_ButtonSaveApp );
	TextButtonHelper(ButtonSaveApp);
	ButtonSaveApp->SetOnClick( SettingsCallback, this );
	ButtonSaveApp->SetIsEnabled( SettingsActiveCallback, this );

	ButtonSaveDefault = new UITextButton( Cinema );
	ButtonSaveDefault->AddToMenu( guiSys, Menu, settingsMenu );
	ButtonSaveDefault->SetLocalPosition( Vector3f( column2, rowpos += rowinc, 0.1f ) );
	ButtonSaveDefault->SetText( CinemaStrings::ButtonText_ButtonSaveDefault );
	TextButtonHelper(ButtonSaveDefault);
	ButtonSaveDefault->SetOnClick( SettingsCallback, this);
	ButtonSaveDefault->SetIsEnabled( SettingsActiveCallback, this );

}

void AppSelectionView::CloseAppButtonPressed()
{
	const PcDef* pc = Cinema.GetCurrentPc();
	const PcDef* app = NULL;

	int selected = MovieBrowser->GetSelection();
	if(selected < AppList.GetSizeI())
	{
		app = AppList[selected];
	}
	if(pc && app)
	{
		Native::closeApp( Cinema.app, pc->UUID, app->Id);
	}
}

void AppSelectionView::SettingsButtonPressed()
{
	int appIndex = MovieBrowser->GetSelection();

	const PcDef* app = AppList[appIndex];

	String	outPath;
	const bool validDir = Cinema.app->GetStoragePaths().GetPathIfValidPermission(
			EST_PRIMARY_EXTERNAL_STORAGE, EFT_FILES, "", W_OK | R_OK, outPath );

	if(validDir)
	{
		if(defaultSettings == NULL)
		{
			defaultSettingsPath = outPath + "settings.json";
			defaultSettings = new Settings(defaultSettingsPath);

			defaultSettings->Define("StreamTheaterSettingsVersion", &settingsVersion);
			defaultSettings->Define("MouseMode", (int*)&mouseMode);
			defaultSettings->Define("StreamWidth", &streamWidth);
			defaultSettings->Define("StreamHeight", &streamHeight);
			defaultSettings->Define("StreamFPS", &streamFPS);
			defaultSettings->Define("EnableHostAudio", &streamHostAudio);
		}

		if(appSettings != NULL)
		{
			delete(appSettings);
			appSettings = NULL;
		}

		appSettingsPath = outPath + "settings." + app->Name + ".json";
		appSettings = new Settings(appSettingsPath);
		appSettings->CopyDefines(*defaultSettings);

		defaultSettings->Load();
		appSettings->Load();
	}

	ButtonGaze->UpdateButtonState();
	ButtonTrackpad->UpdateButtonState();
	ButtonGamepad->UpdateButtonState();
	ButtonOff->UpdateButtonState();
	Button1080->UpdateButtonState();
	Button720->UpdateButtonState();
	Button60FPS->UpdateButtonState();
	Button30FPS->UpdateButtonState();
	ButtonHostAudio->UpdateButtonState();
	ButtonSaveApp->UpdateButtonState();
	ButtonSaveDefault->UpdateButtonState();

	settingsMenu->SetVisible(true);
}

void AppSelectionView::SettingsPressed( UITextButton *button)
{
	if( button->GetText() == CinemaStrings::ButtonText_ButtonGaze )
	{
		mouseMode = MOUSE_GAZE;
	}
	else if( button->GetText() == CinemaStrings::ButtonText_ButtonTrackpad )
	{
		mouseMode = MOUSE_TRACKPAD;
	}
	else if( button->GetText() == CinemaStrings::ButtonText_ButtonGamepad )
	{
		mouseMode = MOUSE_GAMEPAD;
	}
	else if( button->GetText() == CinemaStrings::ButtonText_ButtonOff )
	{
		mouseMode = MOUSE_OFF;
	}
	else if( button->GetText() == CinemaStrings::ButtonText_Button1080 )
	{
		streamWidth = 1920; streamHeight = 1080;
	}
	else if( button->GetText() == CinemaStrings::ButtonText_Button720 )
	{
		streamWidth = 1280; streamHeight = 720;
	}
	else if( button->GetText() == CinemaStrings::ButtonText_Button60FPS )
	{
		streamFPS = 60;
	}
	else if( button->GetText() == CinemaStrings::ButtonText_Button30FPS )
	{
		streamFPS = 30;
	}
	else if( button->GetText() == CinemaStrings::ButtonText_ButtonHostAudio )
	{
		streamHostAudio = !streamHostAudio;
	}
	else if( button->GetText() == CinemaStrings::ButtonText_ButtonSaveApp )
	{
		appSettings->SaveChanged();
	}
	else if( button->GetText() == CinemaStrings::ButtonText_ButtonSaveDefault )
	{
		defaultSettings->SaveAll();
	}

	ButtonGaze->UpdateButtonState();
	ButtonTrackpad->UpdateButtonState();
	ButtonGamepad->UpdateButtonState();
	ButtonOff->UpdateButtonState();
	Button1080->UpdateButtonState();
	Button720->UpdateButtonState();
	Button60FPS->UpdateButtonState();
	Button30FPS->UpdateButtonState();
	ButtonHostAudio->UpdateButtonState();
	ButtonSaveApp->UpdateButtonState();
	ButtonSaveDefault->UpdateButtonState();
}

bool AppSelectionView::SettingsIsSelected( UITextButton *button)
{
	if( button->GetText() == CinemaStrings::ButtonText_ButtonGaze )
	{
		return mouseMode == MOUSE_GAZE;
	}
	else if( button->GetText() == CinemaStrings::ButtonText_ButtonTrackpad )
	{
		return mouseMode == MOUSE_TRACKPAD;
	}
	else if( button->GetText() == CinemaStrings::ButtonText_ButtonGamepad )
	{
		return mouseMode == MOUSE_GAMEPAD;
	}
	else if( button->GetText() == CinemaStrings::ButtonText_ButtonOff )
	{
		return mouseMode == MOUSE_OFF;
	}
	else if( button->GetText() == CinemaStrings::ButtonText_Button1080 )
	{
		return streamWidth == 1920 && streamHeight == 1080;
	}
	else if( button->GetText() == CinemaStrings::ButtonText_Button720 )
	{
		return streamWidth == 1280 && streamHeight == 720;
	}
	else if( button->GetText() == CinemaStrings::ButtonText_Button60FPS )
	{
		return streamFPS == 60;
	}
	else if( button->GetText() == CinemaStrings::ButtonText_Button30FPS )
	{
		return streamFPS == 30;
	}
	else if( button->GetText() == CinemaStrings::ButtonText_ButtonHostAudio )
	{
		return streamHostAudio;
	}
	return false;
}

bool AppSelectionView::SettingsIsActive( UITextButton *button)
{
	if( button->GetText() == CinemaStrings::ButtonText_ButtonSaveApp )
	{
		return appSettings->IsChanged();
	}
	else if( button->GetText() == CinemaStrings::ButtonText_ButtonSaveDefault )
	{
		return defaultSettings->IsChanged();
	}
	return false;
}

Vector3f AppSelectionView::ScalePosition( const Vector3f &startPos, const float scale, const float menuOffset ) const
{
	const float eyeHieght = Cinema.SceneMgr.Scene.GetHeadModelParms().EyeHeight;

	Vector3f pos = startPos;
	pos.x *= scale;
	pos.y = ( pos.y - eyeHieght ) * scale + eyeHieght + menuOffset;
	pos.z *= scale;
	pos += Cinema.SceneMgr.Scene.GetFootPos();

	return pos;
}

//
// Repositions the menu for the lobby scene or the theater
//
void AppSelectionView::UpdateMenuPosition()
{
	// scale down when in a theater
	const float scale = Cinema.InLobby ? 1.0f : 0.55f;
	CenterRoot->GetMenuObject()->SetLocalScale( Vector3f( scale ) );

	if ( !Cinema.InLobby && Cinema.SceneMgr.SceneInfo.UseFreeScreen )
	{
		Quatf orientation = Quatf( Cinema.SceneMgr.FreeScreenPose );
		CenterRoot->GetMenuObject()->SetLocalRotation( orientation );
		CenterRoot->GetMenuObject()->SetLocalPosition( Cinema.SceneMgr.FreeScreenPose.Transform( Vector3f( 0.0f, -1.76f * scale, 0.0f ) ) );
	}
	else
	{
		const float menuOffset = Cinema.InLobby ? 0.0f : 0.5f;
		CenterRoot->GetMenuObject()->SetLocalRotation( Quatf() );
		CenterRoot->GetMenuObject()->SetLocalPosition( ScalePosition( Vector3f::ZERO, scale, menuOffset ) );
	}
}

//============================================================================================

void AppSelectionView::Select()
{
	LOG( "Select");

	// ignore selection while repositioning screen
	if ( RepositionScreen )
	{
		return;
	}

	int lastIndex = MoviesIndex;
	MoviesIndex = MovieBrowser->GetSelection();
	Cinema.SetPlaylist( AppList, MoviesIndex );

	Native::stopAppUpdates(Cinema.app);

	if ( !Cinema.InLobby )
	{
		if ( lastIndex == MoviesIndex )
		{
			// selected the poster we were just watching
			Cinema.ResumeMovieFromSavedLocation();
		}
		else
		{
			Cinema.ResumeOrRestartMovie();
		}
	}
	else
	{
		Cinema.TheaterSelection();
	}
}

void AppSelectionView::StartTimer()
{
	const double now = vrapi_GetTimeInSeconds();
	TimerStartTime = now;
	TimerValue = -1;
	ShowTimer = true;
}

const PcDef *AppSelectionView::GetSelectedApp() const
{
	int selectedItem = MovieBrowser->GetSelection();
	if ( ( selectedItem >= 0 ) && ( selectedItem < AppList.GetSizeI() ) )
	{
		return AppList[ selectedItem ];
	}

	return NULL;
}

void AppSelectionView::SetAppList( const Array<const PcDef *> &movies, const PcDef *nextMovie )
{
	LOG( "SetAppList: %d movies", movies.GetSize() );

	AppList = movies;
	DeletePointerArray( MovieBrowserItems );
	for( UPInt i = 0; i < AppList.GetSize(); i++ )
	{
		const PcDef *movie = AppList[ i ];

		LOG( "AddMovie: %s", movie->Name.ToCStr() );

		CarouselItem *item = new CarouselItem();
		item->texture 		= movie->Poster;
		item->textureWidth 	= movie->PosterWidth;
		item->textureHeight	= movie->PosterHeight;
		MovieBrowserItems.PushBack( item );
	}
	MovieBrowser->SetItems( MovieBrowserItems );

	MovieTitle->SetText( "" );
	LastMovieDisplayed = NULL;

	MoviesIndex = 0;
	if ( nextMovie != NULL )
	{
		for( UPInt i = 0; i < AppList.GetSize(); i++ )
		{
			if ( movies[ i ] == nextMovie )
			{
				StartTimer();
				MoviesIndex = i;
				break;
			}
		}
	}

	MovieBrowser->SetSelectionIndex( MoviesIndex );

	if ( AppList.GetSize() == 0 )
	{
		if ( CurrentCategory == CATEGORY_LIMELIGHT )
		{
			SetError( CinemaStrings::Error_NoApps.ToCStr(), false, false );
		}
		else
		{
			SetError( CinemaStrings::Error_NoVideosOnPhone.ToCStr(), true, false );
		}
	}
	else
	{
		ClearError();
	}
}

void AppSelectionView::SetCategory( const PcCategory category )
{
	// default to category in index 0
	UPInt categoryIndex = 0;
	for( UPInt i = 0; i < Categories.GetSize(); ++i )
	{
		if ( category == Categories[ i ].Category )
		{
			categoryIndex = i;
			break;
		}
	}

	LOG( "SetCategory: %s", Categories[ categoryIndex ].Text.ToCStr() );
	CurrentCategory = Categories[ categoryIndex ].Category;
	for( UPInt i = 0; i < Categories.GetSize(); ++i )
	{
		Categories[ i ].Button->SetHilighted( i == categoryIndex );
	}

	// reset all the swipe icons so they match the current poster
	for( int i = 0; i < NumSwipeTrails; i++ )
	{
		SwipeHintComponent * compLeft = LeftSwipes[ i ]->GetMenuObject()->GetComponentByName<SwipeHintComponent>();
		compLeft->Reset( LeftSwipes[ i ]->GetMenuObject() );
		SwipeHintComponent * compRight = RightSwipes[ i ]->GetMenuObject()->GetComponentByName<SwipeHintComponent>();
		compRight->Reset( RightSwipes[ i ]->GetMenuObject() );
	}

	SetAppList( Cinema.AppMgr.GetAppList( CurrentCategory ), NULL );

	LOG( "%d movies added", AppList.GetSize() );
}

void AppSelectionView::UpdateAppTitle()
{
	const PcDef * currentMovie = GetSelectedApp();
	if ( LastMovieDisplayed != currentMovie )
	{
		if ( currentMovie != NULL )
		{
			MovieTitle->SetText( currentMovie->Name.ToCStr() );
		}
		else
		{
			MovieTitle->SetText( "" );
		}

		LastMovieDisplayed = currentMovie;
	}
}

void AppSelectionView::SelectionHighlighted( bool isHighlighted )
{
	if ( isHighlighted && !ShowTimer && !Cinema.InLobby && ( MoviesIndex == MovieBrowser->GetSelection() ) )
	{
		// dim the poster when the resume icon is up and the poster is highlighted
		CenterPoster->SetColor( Vector4f( 0.55f, 0.55f, 0.55f, 1.0f ) );
	}
	else if ( MovieBrowser->HasSelection() )
	{
		CenterPoster->SetColor( Vector4f( 1.0f ) );
	}
}

void AppSelectionView::UpdateSelectionFrame( const VrFrame & vrFrame )
{
	const double now = vrapi_GetTimeInSeconds();
	if ( !MovieBrowser->HasSelection() )
	{
		SelectionFader.Set( now, 0, now + 0.1, 1.0f );
		TimerStartTime = 0;
	}

	if ( !SelectionFrame->GetMenuObject()->IsHilighted() )
	{
		SelectionFader.Set( now, 0, now + 0.1, 1.0f );
	}
	else
	{
		MovieBrowser->CheckGamepad( Cinema.GetGuiSys(), vrFrame, MovieRoot->GetMenuObject() );
	}

	SelectionFrame->SetColor( Vector4f( SelectionFader.Value( now ) ) );

	int selected = MovieBrowser->GetSelection();
	if ( selected >= 0 && AppList.GetSizeI() > selected && AppList[ selected ]->isRunning )
	{
		ResumeIcon->SetColor( Vector4f( SelectionFader.Value( now ) ) );
		ResumeIcon->SetTextColor( Vector4f( SelectionFader.Value( now ) ) );
		ResumeIcon->SetVisible( true );

		CloseAppButton->SetVisible( true );
	}
	else
	{
		ResumeIcon->SetVisible( false );
		CloseAppButton->SetVisible( false );
	}

	if ( ShowTimer && ( TimerStartTime != 0 ) )
	{
		double frac = ( now - TimerStartTime ) / TimerTotalTime;
		if ( frac > 1.0f )
		{
			frac = 1.0f;
			Cinema.SetPlaylist( AppList, MovieBrowser->GetSelection() );
			Cinema.ResumeOrRestartMovie();
		}
		Vector2f offset( 0.0f, 1.0f - frac );
		TimerIcon->SetColorTableOffset( offset );

		int seconds = TimerTotalTime - ( int )( TimerTotalTime * frac );
		if ( TimerValue != seconds )
		{
			TimerValue = seconds;
			const char * text = StringUtils::Va( "%d", seconds );
			TimerIcon->SetText( text );
		}
		TimerIcon->SetVisible( true );
		CenterPoster->SetColor( Vector4f( 0.55f, 0.55f, 0.55f, 1.0f ) );
	}
	else
	{
		TimerIcon->SetVisible( false );
	}
}

void AppSelectionView::SetError( const char *text, bool showSDCard, bool showErrorIcon )
{
	ClearError();

	LOG( "SetError: %s", text );
	if ( showSDCard )
	{
		SDCardMessage->SetVisible( true );
		SDCardMessage->SetTextWordWrapped( text, Cinema.GetGuiSys().GetDefaultFont(), 1.0f );
	}
	else if ( showErrorIcon )
	{
		ErrorMessage->SetVisible( true );
		ErrorMessage->SetTextWordWrapped( text, Cinema.GetGuiSys().GetDefaultFont(), 1.0f );
	}
	else
	{
		PlainErrorMessage->SetVisible( true );
		PlainErrorMessage->SetTextWordWrapped( text, Cinema.GetGuiSys().GetDefaultFont(), 1.0f );
	}
	TitleRoot->SetVisible( false );
	MovieRoot->SetVisible( false );

	SwipeHintComponent::ShowSwipeHints = false;
}

void AppSelectionView::ClearError()
{
	LOG( "ClearError" );
	ErrorMessageClicked = false;
	ErrorMessage->SetVisible( false );
	SDCardMessage->SetVisible( false );
	PlainErrorMessage->SetVisible( false );
	TitleRoot->SetVisible( true );
	MovieRoot->SetVisible( true );

	SwipeHintComponent::ShowSwipeHints = true;
}

bool AppSelectionView::ErrorShown() const
{
	return ErrorMessage->GetVisible() || SDCardMessage->GetVisible() || PlainErrorMessage->GetVisible();
}

Matrix4f AppSelectionView::DrawEyeView( const int eye, const float fovDegrees )
{
	return Cinema.SceneMgr.DrawEyeView( eye, fovDegrees );
}

Matrix4f AppSelectionView::Frame( const VrFrame & vrFrame )
{
	// We want 4x MSAA in the lobby
	ovrEyeBufferParms eyeBufferParms = Cinema.app->GetEyeBufferParms();
	eyeBufferParms.multisamples = 4;
	Cinema.app->SetEyeBufferParms( eyeBufferParms );

#if 0
	if ( !Cinema.InLobby && Cinema.SceneMgr.ChangeSeats( vrFrame ) )
	{
		UpdateMenuPosition();
	}
#endif

	if ( vrFrame.Input.buttonPressed & BUTTON_B )
	{
		if ( Cinema.InLobby )
		{
			Cinema.app->StartSystemActivity( PUI_CONFIRM_QUIT );
		}
		else
		{
			Cinema.GetGuiSys().CloseMenu( Menu->GetVRMenu(), false );
		}
	}

	// check if they closed the menu with the back button
	if ( !Cinema.InLobby && Menu->GetVRMenu()->IsClosedOrClosing() && !Menu->GetVRMenu()->IsOpenOrOpening() )
	{
		// if we finished the movie or have an error, don't resume it, go back to the lobby
		if ( ErrorShown() )
		{
			LOG( "Error closed.  Return to lobby." );
			ClearError();
			Cinema.AppSelection( true );
		}
		else if ( Cinema.IsMovieFinished() )
		{
			LOG( "Movie finished.  Return to lobby." );
			Cinema.AppSelection( true );
		}
		else
		{
			LOG( "Resume movie." );
			Cinema.ResumeMovieFromSavedLocation();
		}
	}

	if ( !Cinema.InLobby && ErrorShown() )
	{
		SwipeHintComponent::ShowSwipeHints = false;
		if ( vrFrame.Input.buttonPressed & ( BUTTON_TOUCH | BUTTON_A ) )
		{
			Cinema.GetGuiSys().GetApp()->PlaySound( "touch_down" );
		}
		else if ( vrFrame.Input.buttonReleased & ( BUTTON_TOUCH | BUTTON_A ) )
		{
			Cinema.GetGuiSys().GetApp()->PlaySound( "touch_up" );
			ErrorMessageClicked = true;
		}
		else if ( ErrorMessageClicked && ( ( vrFrame.Input.buttonState & ( BUTTON_TOUCH | BUTTON_A ) ) == 0 ) )
		{
			Menu->Close();
		}
	}

	if ( Cinema.SceneMgr.FreeScreenActive && !ErrorShown() )
	{
		if ( !RepositionScreen && !SelectionFrame->GetMenuObject()->IsHilighted() )
		{
			// outside of screen, so show reposition message
			const double now = vrapi_GetTimeInSeconds();
			float alpha = MoveScreenAlpha.Value( now );
			if ( alpha > 0.0f )
			{
				MoveScreenLabel->SetVisible( true );
				MoveScreenLabel->SetTextColor( Vector4f( alpha ) );
			}

			if ( vrFrame.Input.buttonPressed & ( BUTTON_A | BUTTON_TOUCH ) )
			{
				// disable hit detection on selection frame
				SelectionFrame->GetMenuObject()->AddFlags( VRMENUOBJECT_DONT_HIT_ALL );
				RepositionScreen = true;
			}
		}
		else
		{
			// onscreen, so hide message
			const double now = vrapi_GetTimeInSeconds();
			MoveScreenAlpha.Set( now, -1.0f, now + 1.0f, 1.0f );
			MoveScreenLabel->SetVisible( false );
		}

		const Matrix4f invViewMatrix = Cinema.SceneMgr.Scene.CenterViewMatrix().Inverted();
		const Vector3f viewPos( GetViewMatrixPosition( Cinema.SceneMgr.Scene.CenterViewMatrix() ) );
		const Vector3f viewFwd( GetViewMatrixForward( Cinema.SceneMgr.Scene.CenterViewMatrix() ) );

		// spawn directly in front
		Quatf rotation( -viewFwd, 0.0f );
		Quatf viewRot( invViewMatrix );
		Quatf fullRotation = rotation * viewRot;

		const float menuDistance = 1.45f;
		Vector3f position( viewPos + viewFwd * menuDistance );

		MoveScreenLabel->SetLocalPose( fullRotation, position );
	}

	// while we're holding down the button or touchpad, reposition screen
	if ( RepositionScreen )
	{
		if ( vrFrame.Input.buttonState & ( BUTTON_A | BUTTON_TOUCH ) )
		{
			Cinema.SceneMgr.PutScreenInFront();
			Quatf orientation = Quatf( Cinema.SceneMgr.FreeScreenPose );
			CenterRoot->GetMenuObject()->SetLocalRotation( orientation );
			CenterRoot->GetMenuObject()->SetLocalPosition( Cinema.SceneMgr.FreeScreenPose.Transform( Vector3f( 0.0f, -1.76f * 0.55f, 0.0f ) ) );

		}
		else
		{
			RepositionScreen = false;
		}
	}
	else
	{
		// reenable hit detection on selection frame.
		// note: we do this on the frame following the frame we disabled RepositionScreen on
		// so that the selection object doesn't get the touch up.
		SelectionFrame->GetMenuObject()->RemoveFlags( VRMENUOBJECT_DONT_HIT_ALL );
	}

	UpdateAppTitle();
	UpdateSelectionFrame( vrFrame );

	if (Cinema.AppMgr.updated) {
		LOG("Updating App list");
		Cinema.AppMgr.updated = false;
		Cinema.AppMgr.LoadPosters();
		SetAppList(Cinema.AppMgr.GetAppList(CurrentCategory), NULL);
	}

	return Cinema.SceneMgr.Frame( vrFrame );
}

} // namespace VRMatterStreamTheater
