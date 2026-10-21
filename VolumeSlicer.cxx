// =========================================================================
// Base example to interactively cut a volume with a plane
// =========================================================================

#include <fstream>
#include <iostream>
#include <string>

#include <vtkColorTransferFunction.h>
#include <vtkFixedPointVolumeRayCastMapper.h>
#include <vtkInteractorStyleSwitch.h>
#include <vtkMetaImageReader.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkPlaneWidget.h>
#include <vtkCallbackCommand.h>
#include <vtkClipVolume.h>
#include <vtkCommand.h>
#include <vtkPlane.h>
#include <vtkUnstructuredGrid.h>
#include <vtkUnstructuredGridVolumeRayCastMapper.h>

//El Widget del plano simplemente se encarga de interactuar con el plano y la escena. 
//Cuando se va a hacer el clip, vtkClipVolume solo recibe funciones implicitas (vtkWidgetPlane) no es función implicita
//Por eso se necesita este callback para actualizar el plano cuando se interactua con el widget
void UpdatePlane( vtkObject* caller, unsigned long, void* client_data, void* )
{
  auto* widget = static_cast< vtkPlaneWidget* >( caller );
  auto* plane = static_cast< vtkPlane* >( client_data );
  widget->GetPlane( plane );
  std::cout << "origen: " << plane->GetOrigin( )[ 0 ] << " "
  << plane->GetOrigin( )[ 1 ] << " " << plane->GetOrigin( )[ 2 ] << std::endl;
}

int main( int argc, char** argv )
{
  if( argc < 3 )
  {
    std::cout
      << "Usage: " << argv[ 0 ]
      << " file.[mhd|mha] volume_properties.txt"
      << std::endl;
    return( EXIT_FAILURE );
  } // end if
  std::string input_filename = argv[ 1 ];
  std::string vp_filename = argv[ 2 ];

  // Read properties file
  std::ifstream vp_file( vp_filename, std::ios::binary | std::ios::ate );
  if( !vp_file )
  {
    std::cerr << "Could not open file: " << vp_filename << std::endl;
    return( EXIT_FAILURE );
  } // end if
  std::streamsize vp_file_size = vp_file.tellg( );
  vp_file.seekg( 0, std::ios::beg );
  std::string vp( static_cast< std::size_t >( vp_file_size ), '\0' );
  if( !vp_file.read( vp.data( ), vp_file_size ) )
  {
    std::cerr << "Could not read file: " << vp_filename << std::endl;
    return( EXIT_FAILURE );
  } // end if

  // Create volume raycasting properties
  vtkNew< vtkColorTransferFunction > volume_color;
  vtkNew< vtkPiecewiseFunction > volume_opacity;
  vtkNew< vtkPiecewiseFunction > volume_gradient;
  vtkNew< vtkVolumeProperty > volume_properties;
  std::istringstream vp_str( vp );
  std::string line;
  auto trim = []( const std::string& s ) -> std::string
    {
      const char* ws = " \t";
      std::size_t start = s.find_first_not_of( ws );
      if( start == std::string::npos )
        return( "" );
      std::size_t end = s.find_last_not_of( ws );
      return( s.substr( start, end - start + 1 ) );
    };
  while( std::getline( vp_str, line ) )
  {
    std::string l = trim( line );
    if( l != "" )
    {
      std::size_t p = l.find_first_of( " \t" );
      if( p != std::string::npos )
      {
        std::string cmd = l.substr( 0, p );
        std::istringstream values( l.substr( p ) );
        if( cmd == "color" )
        {
          double i, r, g, b;
          values >> i >> r >> g >> b;
          volume_color->AddRGBPoint( i, r, g, b );
        }
        else if( cmd == "opacity" )
        {
          double i, v;
          values >> i >> v;
          volume_opacity->AddPoint( i, v );
        }
        else if( cmd == "gradient" )
        {
          double i, v;
          values >> i >> v;
          volume_gradient->AddPoint( i, v );
        }
        else if( cmd == "shade" )
        {
          std::string v;
          values >> v;
          if( v == "on" )
            volume_properties->ShadeOn( );
          else if( v == "off" )
            volume_properties->ShadeOff( );
        }
        else if( cmd == "ambient" )
        {
          double v;
          values >> v;
          volume_properties->SetAmbient( v );
        }
        else if( cmd == "diffuse" )
        {
          double v;
          values >> v;
          volume_properties->SetDiffuse( v );
        }
        else if( cmd == "specular" )
        {
          double v;
          values >> v;
          volume_properties->SetSpecular( v );
        } // end if
      } // end if
    } // end if
  } // end while

  // Read a 3D image (volume)
  vtkNew< vtkMetaImageReader > volume_reader;
  volume_reader->SetFileName( argv[ 1 ] );

  // Prepare ray-caster
  vtkNew< vtkFixedPointVolumeRayCastMapper > volume_mapper;
  volume_mapper->SetInputConnection( volume_reader->GetOutputPort( ) );

  // Volume rendering properties
  volume_properties->SetColor( volume_color );
  volume_properties->SetScalarOpacity( volume_opacity );
  volume_properties->SetGradientOpacity( volume_gradient );
  volume_properties->SetInterpolationTypeToLinear( );

  // Raycast actor
  vtkNew< vtkVolume > volume;
  volume->SetMapper( volume_mapper );
  volume->SetProperty( volume_properties );

  // Prepare rendering objects
  vtkNew< vtkInteractorStyleSwitch > style;
  vtkNew< vtkPlaneWidget > plane_widget;
  vtkNew< vtkRenderer > ren;
  vtkNew< vtkRenderWindow > win;
  vtkNew< vtkRenderWindowInteractor > iren;


  style->SetCurrentStyleToTrackballCamera( );
  ren->SetBackground( 0.7, 0.7, 0.7 );
  win->AddRenderer( ren );
  iren->SetRenderWindow( win );
  iren->SetInteractorStyle( style );

  volume_reader->Update( );

  //Crea el plano de corte y lo activa, lo relaciona con el interactuador y con el volumen
  plane_widget->SetInteractor( iren );
  plane_widget->SetInputConnection( volume_reader->GetOutputPort( ) );
  plane_widget->SetPlaceFactor( 1.0 );
  plane_widget->PlaceWidget( );
  plane_widget->On();

  // DEclarar el plano que usará el bisturí y obtenerlo del widget
  vtkNew< vtkPlane > plane;
  plane_widget->GetPlane( plane );

  // Hace el corte del volumen
  vtkNew< vtkClipVolume > clip;
  clip->SetInputConnection( volume_reader->GetOutputPort( ) );
  clip->SetClipFunction( plane );
  clip->SetValue( 0.0 );
  clip->InsideOutOn( );
  clip->SetMixed3DCellGeneration( false );

  // Actualizar el plano cuando se suelta el widget
  vtkNew< vtkCallbackCommand > plane_cb;
  plane_cb->SetCallback( UpdatePlane );
  plane_cb->SetClientData( plane.GetPointer( ) );
  plane_widget->AddObserver( vtkCommand::EndInteractionEvent, plane_cb );

  // Mapper y volumen para mostrar el resultado del corte
  vtkNew< vtkUnstructuredGridVolumeRayCastMapper > clip_mapper;
  clip_mapper->SetInputConnection( clip->GetOutputPort( ) );

  vtkNew< vtkVolume > clipped_volume;
  clipped_volume->SetMapper( clip_mapper );
  clipped_volume->SetProperty( volume_properties );

  // Add actors
  ren->AddViewProp( clipped_volume );

  // Finish setup and start main loop
  win->SetSize( 800, 800 );
  win->SetWindowName( "Volume slicer" );
  win->Render( );
  iren->Start( );

  return( EXIT_SUCCESS );
}

// eof - VolumeSlicer.cxx