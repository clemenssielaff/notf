import sys
import apps

if __name__ == "__main__":
    # sys.exit(apps.pane())
    sys.exit(apps.first_test())

# TODO: load application from data
#   I have made a lot of progress to define the whole application as pure data. Most of the application-specific code is
#   defined here, and almost of all it is actually data.
#   There are a few avenues that we need to explore from here:
#   * I want to be able to define the WHOLE Application using one or multiple JSON files with serialized Values.
#       * Then, I want the Application to update in real-time whenever I update one of the relevant files.
#   * We also need better Operators, the ones we have right now are overly constrained. Instead, we need more
#       expressions, and arguments that control a relative small number of build-in Operators.
#   * Maybe get rid of the Operator Index / Layout Index stuff and replace those with names?
#   * Now that NodeDescriptions can be expressed as Values, I want to be able to create new child nodes at runtime and
#       have them placed in the Layout of the parent
#   We also need a lot more improvements on various fronts:
#   * Text drawing
#   * Read Paints (or at least colors) from SVG
#   * Better & More Layouts
#   The biggest unknown as of now is how this would integrate with Services to ensure that facts about the world are
#   updated reliably and in time, but I am fairly certain that this is either a domain-specific problem or already
#   solved and largely independent of the res of the code.
#   Finally, the goal of this whole exercise should be an editor that would allow me to bootstrap the notf design studio
#   from scratch. At first, I can probably start with a text editor and a live-updating application. Then, I would want
#   to use the application to visualize and eventually modify its own logic, so I don't have to use the text editor any
#   more. I suppose, I will have to use inkscape for quite a while for the design, but maybe not. Having the Designs
#   visualized in editor should not be too hard... And once I have that, I hope for a Singularity like event, where the
#   development speed of notf increases due to increased functionality of notf.
#   To the moon, my friends.
