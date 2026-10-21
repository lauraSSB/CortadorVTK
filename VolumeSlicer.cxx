// =========================================================================
// Base example to interactively cut a volume with a plane
// =========================================================================

#include <fstream>
#include <iostream>
#include <string>

#include <vtkActor.h>
#include <vtkAxesActor.h>
#include <vtkButtonWidget.h>
#include <vtkCallbackCommand.h>
#include <vtkClipVolume.h>
#include <vtkColorTransferFunction.h>
#include <vtkCommand.h>
#include <vtkFixedPointVolumeRayCastMapper.h>
#include <vtkImageData.h>
#include <vtkImageShrink3D.h>
#include <vtkInteractorStyleSwitch.h>
#include <vtkMarchingCubes.h>
#include <vtkMetaImageReader.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkOutlineFilter.h>
#include <vtkOutlineSource.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPlane.h>
#include <vtkPlaneWidget.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTextProperty.h>
#include <vtkTextRenderer.h>
#include <vtkTexturedButtonRepresentation2D.h>
#include <vtkUnstructuredGrid.h>
#include <vtkUnstructuredGridVolumeRayCastMapper.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

struct ToggleData {
  vtkVolume *volume;
  vtkActor *mesh;
  vtkRenderWindow *window;
};

void ToggleVolumeMesh(vtkObject *caller, unsigned long, void *client_data,
                      void *) {
  auto *widget = static_cast<vtkButtonWidget *>(caller);
  auto *rep = vtkTexturedButtonRepresentation2D::SafeDownCast(
      widget->GetRepresentation());
  auto *data = static_cast<ToggleData *>(client_data);

  bool showMesh = (rep->GetState() == 1);
  data->volume->SetVisibility(!showMesh);
  data->mesh->SetVisibility(showMesh);
  data->window->Render();
}

// El Widget del plano simplemente se encarga de interactuar con el plano y la
// escena. Cuando se va a hacer el clip, vtkClipVolume solo recibe funciones
// implicitas (vtkWidgetPlane) no es función implicita Por eso se necesita este
// callback para actualizar el plano cuando se interactua con el widget
void UpdatePlane(vtkObject *caller, unsigned long, void *client_data, void *) {
  auto *widget = static_cast<vtkPlaneWidget *>(caller);
  auto *plane = static_cast<vtkPlane *>(client_data);
  widget->GetPlane(plane);
  if (widget->GetInteractor())
    widget->GetInteractor()->Render();

  std::cout << "origen: " << plane->GetOrigin()[0] << " "
            << plane->GetOrigin()[1] << " " << plane->GetOrigin()[2]
            << std::endl;
}

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cout << "Usage: " << argv[0] << " file.[mhd|mha] volume_properties.txt"
              << std::endl;
    return (EXIT_FAILURE);
  } // end if
  std::string input_filename = argv[1];
  std::string vp_filename = argv[2];

  // Read properties file
  std::ifstream vp_file(vp_filename, std::ios::binary | std::ios::ate);
  if (!vp_file) {
    std::cerr << "Could not open file: " << vp_filename << std::endl;
    return (EXIT_FAILURE);
  } // end if
  std::streamsize vp_file_size = vp_file.tellg();
  vp_file.seekg(0, std::ios::beg);
  std::string vp(static_cast<std::size_t>(vp_file_size), '\0');
  if (!vp_file.read(vp.data(), vp_file_size)) {
    std::cerr << "Could not read file: " << vp_filename << std::endl;
    return (EXIT_FAILURE);
  } // end if

  // Create volume raycasting properties
  vtkNew<vtkColorTransferFunction> volume_color;
  vtkNew<vtkPiecewiseFunction> volume_opacity;
  vtkNew<vtkPiecewiseFunction> volume_gradient;
  vtkNew<vtkVolumeProperty> volume_properties;
  std::istringstream vp_str(vp);
  std::string line;
  auto trim = [](const std::string &s) -> std::string {
    const char *ws = " \t";
    std::size_t start = s.find_first_not_of(ws);
    if (start == std::string::npos)
      return ("");
    std::size_t end = s.find_last_not_of(ws);
    return (s.substr(start, end - start + 1));
  };
  while (std::getline(vp_str, line)) {
    std::string l = trim(line);
    if (l != "") {
      std::size_t p = l.find_first_of(" \t");
      if (p != std::string::npos) {
        std::string cmd = l.substr(0, p);
        std::istringstream values(l.substr(p));
        if (cmd == "color") {
          double i, r, g, b;
          values >> i >> r >> g >> b;
          volume_color->AddRGBPoint(i, r, g, b);
        } else if (cmd == "opacity") {
          double i, v;
          values >> i >> v;
          volume_opacity->AddPoint(i, v);
        } else if (cmd == "gradient") {
          double i, v;
          values >> i >> v;
          volume_gradient->AddPoint(i, v);
        } else if (cmd == "shade") {
          std::string v;
          values >> v;
          if (v == "on")
            volume_properties->ShadeOn();
          else if (v == "off")
            volume_properties->ShadeOff();
        } else if (cmd == "ambient") {
          double v;
          values >> v;
          volume_properties->SetAmbient(v);
        } else if (cmd == "diffuse") {
          double v;
          values >> v;
          volume_properties->SetDiffuse(v);
        } else if (cmd == "specular") {
          double v;
          values >> v;
          volume_properties->SetSpecular(v);
        } // end if
      } // end if
    } // end if
  } // end while

  // Read a 3D image (volume)
  vtkNew<vtkMetaImageReader> volume_reader;
  volume_reader->SetFileName(argv[1]);

  // Prepare ray-caster
  vtkNew<vtkFixedPointVolumeRayCastMapper> volume_mapper;
  volume_mapper->SetInputConnection(volume_reader->GetOutputPort());

  // Volume rendering properties
  volume_properties->SetColor(volume_color);
  volume_properties->SetScalarOpacity(volume_opacity);
  volume_properties->SetGradientOpacity(volume_gradient);
  volume_properties->SetInterpolationTypeToLinear();

  // Raycast actor
  vtkNew<vtkVolume> volume;
  volume->SetMapper(volume_mapper);
  volume->SetProperty(volume_properties);

  // Prepare rendering objects
  vtkNew<vtkInteractorStyleSwitch> style;
  vtkNew<vtkPlaneWidget> plane_widget;
  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindow> win;
  vtkNew<vtkRenderWindowInteractor> iren;

  // Create bounding box
  vtkNew<vtkOutlineFilter> outlineFilter;
  outlineFilter->SetInputConnection(volume_reader->GetOutputPort());

  vtkNew<vtkPolyDataMapper> outlineMapper;
  outlineMapper->SetInputConnection(outlineFilter->GetOutputPort());

  vtkNew<vtkActor> outlineActor;
  outlineActor->SetMapper(outlineMapper);
  outlineActor->GetProperty()->SetColor(0.5, 0.0, 0.0);
  outlineActor->GetProperty()->SetLineWidth(2.0);

  style->SetCurrentStyleToTrackballCamera();
  ren->SetBackground(0.7, 0.7, 0.7);
  win->AddRenderer(ren);
  iren->SetRenderWindow(win);
  iren->SetInteractorStyle(style);

  volume_reader->Update();

  // Crea el plano de corte y lo activa, lo relaciona con el interactuador y con
  // el volumen
  plane_widget->SetInteractor(iren);
  plane_widget->SetInputConnection(volume_reader->GetOutputPort());
  plane_widget->SetPlaceFactor(1.0);
  plane_widget->PlaceWidget();
  plane_widget->On();

  // DEclarar el plano que usará el bisturí y obtenerlo del widget
  vtkNew<vtkPlane> plane;
  plane_widget->GetPlane(plane);

  // submuestreo para reducir tiempo de carga
  vtkNew<vtkImageShrink3D> shrink;
  shrink->SetInputConnection(volume_reader->GetOutputPort());
  shrink->SetShrinkFactors(8, 8, 8);
  shrink->AveragingOn();

  // Hace el corte del volumen
  vtkNew<vtkClipVolume> clip;
  clip->SetInputConnection(volume_reader->GetOutputPort());
  clip->SetInputConnection(shrink->GetOutputPort());
  clip->SetClipFunction(plane);
  clip->SetValue(0.0);
  clip->InsideOutOn();
  clip->SetMixed3DCellGeneration(false);

  // Actualizar el plano cuando se suelta el widget
  vtkNew<vtkCallbackCommand> plane_cb;
  plane_cb->SetCallback(UpdatePlane);
  plane_cb->SetClientData(plane.GetPointer());
  plane_widget->AddObserver(vtkCommand::EndInteractionEvent, plane_cb);

  // Mapper y volumen para mostrar el resultado del corte
  vtkNew<vtkUnstructuredGridVolumeRayCastMapper> clip_mapper;
  clip_mapper->SetInputConnection(clip->GetOutputPort());

  vtkNew<vtkVolume> clipped_volume;
  clipped_volume->SetMapper(clip_mapper);
  clipped_volume->SetProperty(volume_properties);

  // Malla
  vtkNew<vtkMarchingCubes> marchingCubes;
  marchingCubes->SetInputConnection(volume_reader->GetOutputPort());
  marchingCubes->SetValue(0, 300.0); // isovalor fijo
  marchingCubes->ComputeNormalsOn();

  vtkNew<vtkPolyDataMapper> meshMapper;
  meshMapper->SetInputConnection(marchingCubes->GetOutputPort());

  vtkNew<vtkActor> meshActor;
  meshActor->SetMapper(meshMapper);
  meshActor->GetProperty()->SetColor(0.9, 0.75, 0.6);
  meshActor->GetProperty()->SetSpecular(0.3);
  meshActor->GetProperty()->SetSpecularPower(20.0);
  meshActor->SetVisibility(0);

  // Orientacion
  vtkNew<vtkAxesActor> axesActor;
  vtkNew<vtkOrientationMarkerWidget> axesWidget;
  axesWidget->SetOutlineColor(0.93, 0.57, 0.13);
  axesWidget->SetOrientationMarker(axesActor);
  axesWidget->SetInteractor(iren);
  axesWidget->SetViewport(0.0, 0.0, 0.2, 0.2);
  axesWidget->SetDefaultRenderer(ren);
  axesWidget->SetEnabled(1);
  axesWidget->InteractiveOn();

  // Boton
  vtkNew<vtkTextProperty> textProp;
  textProp->SetFontSize(24);
  textProp->SetColor(1, 1, 1);
  textProp->SetBackgroundColor(0.1, 0.2, 0.6);
  textProp->SetBackgroundOpacity(1.0);
  textProp->SetJustificationToCentered();
  textProp->SetVerticalJustificationToCentered();

  vtkNew<vtkImageData> texVolumen;
  vtkNew<vtkImageData> texMalla;
  int textDims[2];

  vtkTextRenderer::GetInstance()->RenderString(textProp, "Volumen", texVolumen,
                                               textDims, 96);
  vtkTextRenderer::GetInstance()->RenderString(textProp, "Malla", texMalla,
                                               textDims, 96);

  vtkNew<vtkTexturedButtonRepresentation2D> buttonRep;
  buttonRep->SetNumberOfStates(2);
  buttonRep->SetButtonTexture(0, texVolumen);
  buttonRep->SetButtonTexture(1, texMalla);

  vtkNew<vtkButtonWidget> buttonWidget;
  buttonWidget->SetInteractor(iren);
  buttonWidget->SetRepresentation(buttonRep);
  buttonWidget->SetEnabled(1);

  double buttonBounds[6] = {10.0, 210.0, 730.0, 790.0, 0.0, 0.0};
  buttonRep->PlaceWidget(buttonBounds);

  ToggleData toggleData{clipped_volume.GetPointer(), meshActor.GetPointer(),
                        win.GetPointer()};

  vtkNew<vtkCallbackCommand> buttonCallback;
  buttonCallback->SetCallback(ToggleVolumeMesh);
  buttonCallback->SetClientData(&toggleData);
  buttonWidget->AddObserver(vtkCommand::StateChangedEvent, buttonCallback);

  // Add actors
  ren->AddViewProp(clipped_volume);
  ren->AddViewProp(meshActor);
  ren->AddViewProp(outlineActor);

  // Centrar camara en escena
  ren->ResetCamera();

  // Finish setup and start main loop
  win->SetSize(600, 600);
  win->SetWindowName("Volume slicer");
  win->Render();
  iren->Start();

  return (EXIT_SUCCESS);
}

// eof - VolumeSlicer.cxx