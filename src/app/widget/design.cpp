#include "app/widget/design.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

WidgetDesign::CommandBase::~CommandBase() = default;

WidgetDesign::SetWindingCommand::~SetWindingCommand() = default;
WidgetDesign::MoveCommand::~MoveCommand() = default;
WidgetDesign::LineCommand::~LineCommand() = default;
WidgetDesign::BezierCommand::~BezierCommand() = default;
WidgetDesign::SetTransformCommand::~SetTransformCommand() = default;
WidgetDesign::TransformCommand::~TransformCommand() = default;
WidgetDesign::TranslationCommand::~TranslationCommand() = default;
WidgetDesign::RotationCommand::~RotationCommand() = default;
WidgetDesign::SetClippingCommand::~SetClippingCommand() = default;
WidgetDesign::FillColorCommand::~FillColorCommand() = default;
WidgetDesign::FillPaintCommand::~FillPaintCommand() = default;
WidgetDesign::StrokeColorCommand::~StrokeColorCommand() = default;
WidgetDesign::StrokePaintCommand::~StrokePaintCommand() = default;
WidgetDesign::StrokeWidthCommand::~StrokeWidthCommand() = default;
WidgetDesign::BlendModeCommand::~BlendModeCommand() = default;
WidgetDesign::SetAlphaCommand::~SetAlphaCommand() = default;
WidgetDesign::MiterLimitCommand::~MiterLimitCommand() = default;
WidgetDesign::LineCapCommand::~LineCapCommand() = default;
WidgetDesign::LineJoinCommand::~LineJoinCommand() = default;
WidgetDesign::WriteCommand::~WriteCommand() = default;

NOTF_CLOSE_NAMESPACE
