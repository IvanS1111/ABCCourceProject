/**
 * module.h - module template
 * @author Pavel Kryukov
 * Copyright 2019 MIPT-V team
 */

#ifndef INFRA_PORTS_MODULE_H
#define INFRA_PORTS_MODULE_H
 
#include <infra/log.h>
#include <infra/ports/ports.h>

#include <boost/property_tree/ptree_fwd.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/optional/optional.hpp>
#include <map>

#include <set>
#include <unordered_set>

#include <modules/core/perf_instr.h>

class Module : public Log
{
public:
    Module( Module* parent, std::string name);

    void static writeFile(std::string file)
    {
        boost::property_tree::ptree arr;
        if(file.length())
        {
            for(auto i : json_record)
            {
                for (auto j : i.second.get_child("stages"))
                    arr.push_back(j);
                for (auto j : i.second.get_child("records"))
                    arr.push_back(j);
                for (auto j : i.second.get_child("events"))
                    arr.push_back(j);

                boost::property_tree::ptree write_data;
                write_data.add_child("", arr);
                boost::property_tree::write_json(file + ".json", write_data, std::locale(), false);
            }
        }
    }

protected:
    template<typename T>
    auto make_write_port( std::string key, uint32 bandwidth) 
    {
        auto port = std::make_unique<WritePort<T>>( get_portmap(), std::move(key), bandwidth);
        auto ptr = port.get();
        write_ports.emplace_back( std::move( port));
        return ptr;
    }


    template<typename T>
    auto make_read_port( std::string key, Latency latency)
    {
        auto port = std::make_unique<ReadPort<T>>( get_portmap(), std::move(key), latency);
        auto ptr = port.get();
        read_ports.emplace_back( std::move( port));
        return ptr;
    }
    template <typename FuncInstr>
    void recordStage(const PerfInstr<FuncInstr> &instr, std::string description, Cycle cycle)
    {
        boost::property_tree::ptree write, write_stage_vector, write_event_vector;
        if(description == "Fetch")
        {
            boost::property_tree::ptree &pointer = json_record[instr.get_sequence_id()];
            pointer.put_child("records", write);
            pointer.put_child("stages", write_stage_vector);
            pointer.put_child("events", write_event_vector);

            write.put("type", "Record");
            write.put("id", instr.get_sequence_id());
            write.put("disassembly", instr.get_mask());

            boost::optional<boost::property_tree::ptree &> child0 = pointer.get_child_optional("records");
            child0 -> push_back(boost::property_tree::ptree::value_type("", write));

        }


        int id;
        if(description == "Fetch")
            id = 0;
        else
        if(description == "Decode")
            id = 1;
        else
        if(description == "Execute")
            id = 2;
        else
        if(description == "Memory")
            id = 3;
        else
        if(description == "Writeback")
            id = 4;

        write_stage_vector.put("type", "Stage");
        write_stage_vector.put("id", id);
        write_stage_vector.put("description", description);

        write_event_vector.put("type", "event");
        write_event_vector.put("id", instr.get_sequence_id());
        write_event_vector.put("cycle", cycle);
        write_event_vector.put("stage", id);


        boost::property_tree::ptree &pointer = json_record[instr.get_sequence_id()];
        boost::optional<boost::property_tree::ptree &> child1 = pointer.get_child_optional("stages");
        child1 -> push_back(boost::property_tree::ptree::value_type("", write_stage_vector));
        boost::optional<boost::property_tree::ptree &> child2 = pointer.get_child_optional("events");
        child2 -> push_back(boost::property_tree::ptree::value_type("", write_event_vector));

    }

    void enable_logging_impl( const std::unordered_set<std::string>& names);
    boost::property_tree::ptree topology_dumping_impl() const;

private:
    // NOLINTNEXTLINE(misc-no-recursion) Recursive, but must be finite
    virtual std::shared_ptr<PortMap> get_portmap() const { return parent->get_portmap(); }
    void force_enable_logging();
    void force_disable_logging();

    void add_child( Module* module) { children.push_back( module); }

    virtual boost::property_tree::ptree portmap_dumping() const;
    boost::property_tree::ptree read_ports_dumping() const;
    boost::property_tree::ptree write_ports_dumping() const;

    void module_dumping( boost::property_tree::ptree* modules) const;
    void modulemap_dumping( boost::property_tree::ptree* modulemap) const;

    Module* const parent;
    std::vector<Module*> children;
    const std::string name;
    std::vector<std::unique_ptr<BasicWritePort>> write_ports;
    std::vector<std::unique_ptr<BasicReadPort>> read_ports;
    static std::map<int, boost::property_tree::ptree> json_record;
};

class Root : public Module
{
public:
    explicit Root( std::string name)
        : Module( nullptr, std::move( name))
        , portmap( PortMap::create_port_map())
    { }

protected:
    void init_portmap() { portmap->init(); }
    void enable_logging( const std::string& values);
    
    void topology_dumping( bool dump, const std::string& filename);

private:
    std::shared_ptr<PortMap> get_portmap() const final { return portmap; }
    std::shared_ptr<PortMap> portmap;
    boost::property_tree::ptree portmap_dumping() const final;
};

#endif
