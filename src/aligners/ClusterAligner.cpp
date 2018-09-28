/*
	This file is part of Overmix.

	Overmix is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Overmix is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Overmix.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "ClusterAligner.hpp"
#include "../comparators/AComparator.hpp"
#include "../containers/AContainer.hpp"
#include "../utils/AProcessWatcher.hpp"

#include <algorithm>
#include <cassert>
#include <numeric>
#include <stdexcept>
#include <vector>
#include <iostream> //For debug

using namespace Overmix;

struct Cluster{
	AContainer& container;
	unsigned amount;	
	
	// clusters[ image_index ] -> belongs to cluser. -1 if not yet asigned to cluster
	std::vector<int> clusters;
	
	Cluster( AContainer& container, unsigned amount )
		: container(container), amount(amount), clusters( container.count(), -1 ) { }

	
	unsigned count( int cluster_id ) const{
		return std::count_if( clusters.begin(), clusters.end()
			,	[=]( auto cluster ){ return cluster == cluster_id; }
			);
	}
	
	unsigned remaining() const{ return count( -1 ); }
	
	bool isInCluster( int cluster, unsigned image ) const
		{ return clusters[image] == cluster; }
	
	unsigned withoutCluster() const{
		for( unsigned i=0; i<clusters.size(); i++ )
			if( clusters[i] == -1 )
				return i;
		throw std::logic_error( "withoutCluster called while all images had clusters" );
	}
	
	void assign(){
		while( remaining() > 0 ){
			int best_cluster = -1;
			unsigned best_image = 0;
			double best_error = std::numeric_limits<double>::max();
			
			// Check for clusters without images
			for( unsigned id=0; id<amount; id++ )
				if( count( id ) == 0 ){
					//Assign the missing cluster to a random image not belonging to a cluster
					clusters[withoutCluster()] = id;
					goto cluster_assigned;
				}
			
			// Find the lowest error from a image in a cluster, to an image outside that cluster
			for( unsigned from=0; from<clusters.size(); from++ )
				for( unsigned to=0; to<clusters.size(); to++ ){
					auto this_cluster = clusters[from];
					if( this_cluster == -1 || isInCluster( this_cluster, to ) )
						continue;
					
					auto offset = container.getCachedOffset( from, to );
					if( offset.error < best_error ){
						best_error = offset.error;
						best_cluster = this_cluster;
						best_image = to;
					}
				}
			
			// If this is an image without a cluster, assign it to this cluster
			if( clusters[best_image] == -1 )
				clusters[best_image] = best_cluster;
			else{
				// If it belongs to another cluster, combine these two clusters
				for( auto& cluster : clusters )
					if( cluster == best_cluster )
						cluster = clusters[best_image];
			}
			
	cluster_assigned:;
		}
		
		//Check if all clusters have images
		for( unsigned id=0; id<amount; id++ )
			assert( count( id ) > 0 );
	}
	
	void setFrames(){
		std::vector<int> frame_ids( amount, -1 );
		int found = 0;
		
		// Reorder the cluster ids so first occurance of an id is always higher than any seen before
		for( auto& cluster : clusters )
			if( frame_ids[cluster] == -1 )
				frame_ids[cluster] = found++;
			
		for( unsigned i=0; i<container.count(); i++ )
			container.setFrame( i, frame_ids[clusters[i]] );
	}
};

void ClusterAligner::align( AContainer& container, AProcessWatcher* watcher ) const {
	Progress progress( "ClusterAligner", container.count() * container.count(), watcher );
	for( unsigned i=0; i<container.count(); i++ )
		for( unsigned j=0; j<container.count(); j++ ){
			container.findOffset( i, j );
			progress.add();
		}
	
	// Find the clustering which gives the best balance between amount of groups and distance
	Cluster best( container, min_groups );
	best.assign();
	for( unsigned frames=min_groups+1; frames<=max_groups; frames++ ){
		Cluster c( container, frames );
		c.assign();
		//TODO: evaluate
		if( true ){
			best.amount   = c.amount;
			best.clusters = c.clusters;
		}
	}
	
	//With the best cluster, set frame ids
	best.setFrames();
}

