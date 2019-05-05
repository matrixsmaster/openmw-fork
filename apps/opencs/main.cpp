#include "editor.hpp"

#include <exception>
#include <string>

#include <QApplication>
#include <QIcon>
#include <QMetaType>

#include <components/debug/debugging.hpp>

#include "model/doc/messages.hpp"
#include "model/world/universalid.hpp"

Q_DECLARE_METATYPE (std::string)

class Application : public QApplication
{
    private:

        bool notify (QObject *receiver, QEvent *event)
        {
            try
            {
                return QApplication::notify (receiver, event);
            }
            catch (const std::exception& exception)
            {
            	Log(Debug::Error) << "An exception has been caught: " << exception.what();
            }

            return false;
        }

    public:

        Application (int& argc, char *argv[]) : QApplication (argc, argv) {}
};

int main(int argc, char *argv[])
{
    // To allow background thread drawing in OSG
    QApplication::setAttribute(Qt::AA_X11InitThreads, true);

    Q_INIT_RESOURCE (resources);

    qRegisterMetaType<std::string> ("std::string");
    qRegisterMetaType<CSMWorld::UniversalId> ("CSMWorld::UniversalId");
    qRegisterMetaType<CSMDoc::Message> ("CSMDoc::Message");

    Application application (argc, argv);

    application.setWindowIcon (QIcon (":./openmw-cs.png"));

    CS::Editor editor(argc, argv);

    if(!editor.makeIPCServer())
    {
        editor.connectToIPCServer();
        return 0;
    }

    return editor.run();
}
