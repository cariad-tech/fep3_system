/**
 * @file
 * @copyright
 * @verbatim
Copyright @ 2021 VW Group. All rights reserved.

    This Source Code Form is subject to the terms of the Mozilla
    Public License, v. 2.0. If a copy of the MPL was not distributed
    with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

If it is not possible or desirable to put the notice in a particular file, then
You may include the notice in a location (such as a LICENSE file in a
relevant directory) where a recipient would be likely to look for such a notice.

You may add additional accurate notices of copyright ownership.

@endverbatim
 */


#include <gtest/gtest.h>
#include <fep_system/fep_system.h>
#include <string.h>
#include <fep_test_common.h>
#include "a_util/logging.h"
#include "a_util/process.h"
#include <boost/dll.hpp>

#include <a_util/xml.h>
#include "component_file_paths.h"
#include <functional>
#include <clipp.h>

int cli_argc = 0;
char** cli_argv = nullptr;

namespace
{
    static const std::string component_file_env_variable = "FEP3_PARTICIPANT_COMPONENTS_FILE_PATH";
    static const std::string dds_service_discovery_command_line_arg = "dds_service_discovery";
    static const std::string http_service_discovery_command_line_arg = "http_service_discovery";

    void replaceServiceBusComponents(const std::string& file_name, const std::string& component_path)
    {

        boost::filesystem::path file_path(file_name);

        ASSERT_TRUE(boost::filesystem::exists(file_path));

        std::ifstream component_file;

        a_util::xml::DOM loaded_file;
        if (loaded_file.load(file_path.string()))
        {
            a_util::xml::DOMElementList components;
            if (loaded_file.getRoot().findNodes("component", components))
            {
                for (auto& component : components)
                {
                    auto iid = component.getChild("iid");
                    auto iid_value = iid.getData();
                    if (iid_value.find("service_bus") != std::string::npos)
                    {
                        auto source = component.getChild("source");
                        source.setData(component_path);
                    }
                }
            }
        }
        loaded_file.save(file_path.string());
    }

    std::string getPluginFromCommandLine()
    {
        std::string pluging_argument;

        auto cli = (
            clipp::option
            ( "--plugin")
            & clipp::value("milliseconds", pluging_argument)
            .doc("service discovery plugin to be used for testing"));

        auto result = clipp::parse(cli_argc, cli_argv, cli);

        if (!result)
        {
            std::cout << "Error in command line: ";
            clipp::debug::print(std::cerr, result);
            std::cout << std::endl << std::endl;
            std::cout << "Default Command line argument will be used " << "\n";
            return dds_service_discovery_command_line_arg;
        }
        else if (pluging_argument.empty())
        {
            std::cout << "Command line argument empty, default Command line argument will be used " << "\n";
            return dds_service_discovery_command_line_arg;
        }
        else
        {
            std::cout << "Command line argument " << pluging_argument << "\n";
            return pluging_argument;
        }
    }
}
/**
 * @req_id FEPSDK-2128
 */
/*
TEST_F(TestSystemDiscovery, StandaloneModulesWillNotBeDiscovered)
{
    const auto participant_names = std::vector<std::string>
        { "standalone_participant"
        , "participant_no_standalone_path"
        , "participant3" };

    const Modules modules = createTestModules(participant_names);

    ASSERT_EQ(
        modules.at("standalone_participant")->GetPropertyTree()->SetPropertyValue(FEP_STM_STANDALONE_PATH, true),
        a_util::result::Result());

    ASSERT_EQ(
        modules.at("participant_no_standalone_path")->GetPropertyTree()->DeleteProperty(FEP_STM_STANDALONE_PATH),
        a_util::result::Result());

    /// discover
    const auto domain_id = modules.begin()->second->GetDomainId();
    fep::System my_system = fep::discoverSystemOnDDS("my_system", domain_id, 1000);
    const auto discovered_participants = my_system.getParticipants();

    /// compare with expectation
    const std::vector<std::string> discoverd_names = [discovered_participants]() {
        std::vector<std::string> temp;
        std::transform
        (discovered_participants.begin()
            , discovered_participants.end()
            , std::back_inserter(temp)
            , [](decltype(discovered_participants)::value_type participant)
        {
            return participant.getName();
        }
        );
        return temp;
    }();
    EXPECT_EQ(discoverd_names, std::vector<std::string>({"participant3", "participant_no_standalone_path"}));
}*/

void testDiscoverSystem(const fep3::System& system, const std::vector<std::string>& participant_names)
{
    const auto discovered_participants = system.getParticipants();
    /// compare with expectation
    std::vector<std::string> discovered_names;
    std::transform
    (discovered_participants.begin()
        , discovered_participants.end()
        , std::back_inserter(discovered_names)
        , [&](const auto& participant)
        {
            return participant.getName();
        }
    );

    EXPECT_EQ(discovered_names, participant_names);
}

class TestSystemDiscovery :public ::testing::Test {
protected:

    void SetUp() override
    {
        _plugin_under_test = getPluginFromCommandLine();
        std::string service_bus_component_plugin;
        std::string system_plugin;
        if (_plugin_under_test == dds_service_discovery_command_line_arg)
        {
            service_bus_component_plugin = "./rti/fep3_dds_service_bus_plugin";
#ifdef __linux__
            system_plugin = "./rti/libfep3_dds_service_bus_plugin.so";
#elif _WIN32
            system_plugin = "./rti/fep3_dds_service_bus_plugin.dll";
#endif
        }
        else if (_plugin_under_test == http_service_discovery_command_line_arg)
        {
            service_bus_component_plugin = "./fep_components_plugin";
#ifdef __linux__
            system_plugin = "./http/libfep3_http_service_bus.so";
#elif _WIN32
            system_plugin = "./http/fep3_http_service_bus.dll";
#endif
        }
        else
        {
            throw std::runtime_error("Unknown service bus plugin to test");
        }

        copyComponentFile(_participant_component_plugins_file_path, _participant_copy_component_plugins_file_path);
        copyComponentFile(_system_plugins_file_path, _system_plugins_copy_file_path);
        replaceServiceBusComponents(_participant_component_plugins_file_path, service_bus_component_plugin);
        replaceServiceBusComponents(_system_plugins_file_path, system_plugin);
        a_util::process::setEnvVar(component_file_env_variable, _participant_component_plugins_file_path);
    }

    void TearDown()
    {
        //restore the files to original
        boost::filesystem::copy_file(_system_plugins_copy_file_path, _system_plugins_file_path, boost::filesystem::copy_option::overwrite_if_exists);
        boost::filesystem::copy_file(_participant_copy_component_plugins_file_path, _participant_component_plugins_file_path, boost::filesystem::copy_option::overwrite_if_exists);
        // remove the copied files
        boost::filesystem::remove(_system_plugins_copy_file_path);
        boost::filesystem::remove(_participant_copy_component_plugins_file_path);
        a_util::process::setEnvVar(component_file_env_variable, "");
    }

    std::string _plugin_under_test;
private:
    void copyComponentFile(const std::string& source_file_path, std::string& copied_file_path)
    {
        ASSERT_TRUE(boost::filesystem::exists(source_file_path));

        boost::filesystem::path copied_file = boost::filesystem::path(source_file_path + "_" + _backup_file_postfix);
        copied_file_path = copied_file.string();
        boost::filesystem::copy_file(source_file_path, copied_file, boost::filesystem::copy_option::overwrite_if_exists);
    }

    const std::string _backup_file_postfix = "backup";
    const std::string _system_plugins_file_path{system_plugin_file_path};
    const std::string _participant_component_plugins_file_path{components_file_path};
    std::string _system_plugins_copy_file_path;
    std::string _participant_copy_component_plugins_file_path;
};

/**
 * @brief The discovery of participants is tested
 * @req_id
 */
TEST_F(TestSystemDiscovery, DiscoverSystem)
{
    using namespace std::literals::chrono_literals;
    auto sys_name = makePlatformDepName("test_system");
    const auto participant_names = std::vector<std::string>{ "participant1", "participant2" };
    const TestParticipants test_parts = createTestParticipants(participant_names, sys_name);

    ASSERT_NO_FATAL_FAILURE(testDiscoverSystem(fep3::discoverSystem(sys_name, 5000ms), participant_names));

    ASSERT_NO_FATAL_FAILURE(testDiscoverSystem(fep3::discoverSystem(sys_name,2, 3000ms), participant_names));
    ASSERT_NO_FATAL_FAILURE(testDiscoverSystem(fep3::discoverSystem(sys_name, participant_names, 3000ms), participant_names));

    ASSERT_THROW(fep3::discoverSystem(sys_name, 3, 1000ms), std::exception);
    ASSERT_THROW(fep3::discoverSystem(sys_name, { "participant1", "participant2" ,  "participant3" }, 1000ms), std::exception);
}

class TestSystemsDiscovery : public TestSystemDiscovery
{
protected:

    void checkDiscoveredSystems(const std::vector<fep3::System>& discovered_systems)
    {
        bool system_1_found = false;
        bool system_2_found = false;
        bool system_3_found = false;

        //we can not assure that absolutely no other system is disovered here
        //all defaults using the same discovery url!!
        ASSERT_TRUE(discovered_systems.size() >= 3);

        for (auto const& current_system : discovered_systems)
        {
            const auto discovered_participants = current_system.getParticipants();
            decltype(participant_names_1)* current_names = nullptr;
            if (current_system.getSystemName() == sys_name_1)
            {
                system_1_found = true;
                current_names = &participant_names_1;
            }
            else if (current_system.getSystemName() == sys_name_2)
            {
                system_2_found = true;
                current_names = &participant_names_2;
            }
            else if (current_system.getSystemName() == sys_name_3)
            {
                system_3_found = true;
                current_names = &participant_names_3;
            }
            if (current_names)
            {
                /// compare with expectation
                std::vector<std::string> discovered_names;
                std::transform(
                    discovered_participants.begin()
                    , discovered_participants.end()
                    , std::back_inserter(discovered_names)
                    , [](const auto& participant)
                    {
                        return participant.getName();
                    }
                );

                EXPECT_TRUE(std::is_permutation(discovered_names.begin(), discovered_names.end(), current_names->begin()));
            }
        }

        ASSERT_TRUE(system_1_found);
        ASSERT_TRUE(system_2_found);
        ASSERT_TRUE(system_3_found);
    }

    const std::string sys_name_1 = makePlatformDepName("test_system_number_1");
    const std::vector<std::string> participant_names_1{ "participant1", "participant2" };
    const std::string sys_name_2 = makePlatformDepName("test_system_number_2");
    const std::vector<std::string> participant_names_2 = std::vector<std::string>{ "participant1", "participant2" };
    const std::string sys_name_3 = makePlatformDepName("test_system_number_3");
    const std::vector<std::string> participant_names_3{ "part_x", "part_y", "part_z" };
};

/**
 * @brief The discovery of participants is tested
 * @req_id
 */
TEST_F(TestSystemsDiscovery, DiscoverSystemAll)
{
    const TestParticipants test_parts_1 = createTestParticipants(participant_names_1, sys_name_1);
    const TestParticipants test_parts_2 = createTestParticipants(participant_names_2, sys_name_2);
    const TestParticipants test_parts_3 = createTestParticipants(participant_names_3, sys_name_3);

    /// discover system
    std::vector<fep3::System> my_systems;

    // dds may need slightly more time, this overload is unsafe and should be avoided
    if (_plugin_under_test == dds_service_discovery_command_line_arg)
    {
        using namespace std::literals::chrono_literals;
        ASSERT_NO_THROW(
            my_systems = std::move(fep3::discoverAllSystems(2000ms));
        );
    }
    else
    {
        ASSERT_NO_THROW(
            my_systems = std::move(fep3::discoverAllSystems());
        );
    }
    checkDiscoveredSystems(my_systems);

    using namespace std::literals::chrono_literals;

    my_systems.clear();
    ASSERT_NO_THROW(my_systems = fep3::discoverAllSystems(7, 5000ms));
    checkDiscoveredSystems(my_systems);
}


int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    cli_argc = argc;
    cli_argv = argv;
    return RUN_ALL_TESTS();
}
