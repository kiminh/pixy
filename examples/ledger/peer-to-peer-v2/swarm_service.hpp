#ifndef SWARM_SERVICE_HPP
#define SWARM_SERVICE_HPP

#include"service/server.hpp"
#include"network/tcp_server.hpp"
#include"protocols.hpp"
#include"commandline/parameter_parser.hpp"

#include"http/server.hpp"
#include"http/middleware/allow_origin.hpp"
#include"http/middleware/color_log.hpp"

#include<set>

class FetchSwarmService : public fetch::protocols::SwarmProtocol  
{
public:
  FetchSwarmService(uint16_t port, uint16_t http_port, std::string const& pk,
    fetch::network::ThreadManager *tm ) :
    fetch::protocols::SwarmProtocol( tm,  fetch::protocols::FetchProtocols::SWARM, details_),
    thread_manager_( tm ),
    service_(port, thread_manager_),
    http_server_(http_port, thread_manager_)
  {
    using namespace fetch::protocols;
    
    std::cout << "Listening for peers on " << (port) << ", clients on " << (http_port ) << std::endl;

    details_.with_details([=]( NodeDetails &  details) {
        details.public_key = pk;
        details.default_port = port;
        details.default_http_port = http_port;
      });

    EntryPoint e;
    // At this point we don't know what the IP is, but localhost is one entry point    
    e.host = "127.0.0.1"; 
    e.shard = 0;

    e.port = details_.default_port();
    e.http_port = details_.default_http_port();
    e.configuration = EntryPoint::NODE_SWARM;      
    details_.AddEntryPoint(e); 

    running_ = false;
    
    start_event_ = thread_manager_->OnAfterStart([this]() {
        running_ = true;        
        thread_manager_->io_service().post([this]() {
            this->TrackPeers();
          });
      });

    stop_event_ = thread_manager_->OnBeforeStop([this]() {
        running_ = false;
      });
    
    

    service_.Add(fetch::protocols::FetchProtocols::SWARM, this);

    // Setting callback to resolve IP
    this->SetClientIPCallback([this](uint64_t const &n) -> std::string {
        return service_.GetAddress(n);        
      });

    // Creating a http server based on the swarm protocol
    http_server_.AddMiddleware( fetch::http::middleware::AllowOrigin("*") );       
    http_server_.AddMiddleware( fetch::http::middleware::ColorLog);
    http_server_.AddModule(*this);
    
  }

  ~FetchSwarmService() 
  {

  }

  void TrackPeers() 
  {
    using namespace fetch::protocols;
    
    std::this_thread::sleep_for(std::chrono::milliseconds( 2000 ) );
    std::set< fetch::byte_array::ByteArray > public_keys;    
    public_keys.insert(this->details_.details().public_key);
    

    // Finding keys to those we are connected to
    this->with_server_details_do([&](std::map< uint64_t, NodeDetails > const & details) {
        for(auto const &d: details)
        {
          public_keys.insert( d.second.public_key );          
        }
        
      });

    this->with_client_details_do([&](std::map< uint64_t, NodeDetails > const & details) {
        for(auto const &d: details)
        {
          public_keys.insert( d.second.public_key );          
        }        
      });
    
    // Finding hosts we are not connected to
    std::vector< EntryPoint > swarm_entries;    
    this->with_suggestions_do([=, &swarm_entries](std::vector< NodeDetails > const &details) {
        for(auto const &d: details)
        {
          if( public_keys.find( d.public_key ) == public_keys.end() )
          {
            for(auto const &e: d.entry_points)
            {
              if(e.configuration & EntryPoint::NODE_SWARM)
              {
                swarm_entries.push_back( e );
              }
            }
          }
        }
      });

    std::random_shuffle(swarm_entries.begin(), swarm_entries.end());    
    std::cout << "I wish to connect to: " << std::endl;
    std::size_t i = public_keys.size();

    std::size_t desired_connectivity_  = 5;   
    for(auto &e : swarm_entries) {
      std::cout << " - " << e.host << ":" << e.port << std::endl;
      this->Bootstrap(e.host, e.port);
      
      ++i;      
      if(i > desired_connectivity_ )
      {
        break;        
      }
    }
    
    if(running_) {
      thread_manager_->io_service().post([this]() {
          this->TrackPeers();          
        });    
    }    
    
  }
  

private:
  fetch::network::ThreadManager *thread_manager_;    
  fetch::service::ServiceServer< fetch::network::TCPServer > service_;
  fetch::http::HTTPServer http_server_;  
  
//  fetch::protocols::SwarmProtocol *swarm_ = nullptr;
  fetch::protocols::SharedNodeDetails details_;

  typename fetch::network::ThreadManager::event_handle_type start_event_;
  typename fetch::network::ThreadManager::event_handle_type stop_event_;  
  std::atomic< bool > running_;  
};

#endif
