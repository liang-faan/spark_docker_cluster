// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __RESOURCES_HPP__
#define __RESOURCES_HPP__

#include <map>
#include <iosfwd>
#include <set>
#include <string>
#include <vector>

#include <boost/iterator/indirect_iterator.hpp>

#include <google/protobuf/repeated_field.h>

#include <mesos/mesos.hpp>
#include <mesos/resource_quantities.hpp>
#include <mesos/type_utils.hpp>
#include <mesos/values.hpp>

#include <stout/bytes.hpp>
#include <stout/check.hpp>
#include <stout/error.hpp>
#include <stout/foreach.hpp>
#include <stout/hashmap.hpp>
#include <stout/json.hpp>
#include <stout/lambda.hpp>
#include <stout/nothing.hpp>
#include <stout/option.hpp>
#include <stout/try.hpp>


// Resources come in three types: scalar, ranges, and sets. These are
// represented using protocol buffers. To make manipulation of
// resources easier within the Mesos core and for scheduler writers,
// we provide generic overloaded operators (see below) as well as a
// general Resources class that encapsulates a collection of protocol
// buffer Resource objects. The Resources class also provides a few
// static routines to allow parsing resources (e.g., from the command
// line), as well as determining whether or not a Resource object is
// valid. Note that many of these operations have not been optimized
// but instead just written for correct semantics.

namespace mesos {

// Forward declaration.
class ResourceConversion;


// Helper functions.
bool operator==(
    const Resource::ReservationInfo& left,
    const Resource::ReservationInfo& right);


bool operator!=(
    const Resource::ReservationInfo& left,
    const Resource::ReservationInfo& right);


// NOTE: Resource objects stored in the class are always valid, are in
// the "post-reservation-refinement" format, and kept combined if possible.
// It is the caller's responsibility to validate any Resource object or
// repeated Resource protobufs before constructing a Resources object.
// Otherwise, invalid Resource objects will be silently stripped.
// Invalid Resource objects will also be silently ignored when used in
// arithmetic operations (e.g., +=, -=, etc.).
class Resources
{
private:
  // An internal abstraction to facilitate managing shared resources.
  // It allows 'Resources' to group identical shared resource objects
  // together into a single 'Resource_' object and tracked by its internal
  // counter. Non-shared resource objects are not grouped.
  //
  // The rest of the private section is below the public section. We
  // need to define Resource_ first because the public typedefs below
  // depend on it.
  class Resource_
  {
  public:
    /*implicit*/ Resource_(const Resource& _resource)
      : resource(_resource),
        sharedCount(None())
    {
      // Setting the counter to 1 to denote "one copy" of the shared resource.
      if (resource.has_shared()) {
        sharedCount = 1;
      }
    }

    /*implicit*/ Resource_(Resource&& _resource)
      : resource(std::move(_resource)),
        sharedCount(None())
    {
      // Setting the counter to 1 to denote "one copy" of the shared resource.
      if (resource.has_shared()) {
        sharedCount = 1;
      }
    }

    Resource_(const Resource_& resource_) = default;
    Resource_(Resource_&& resource_) = default;

    Resource_& operator=(const Resource_&) = default;
    Resource_& operator=(Resource_&&) = default;

    // By implicitly converting to Resource we are able to keep Resource_
    // logic internal and expose only the protobuf object.
    operator const Resource&() const { return resource; }

    // Check whether this Resource_ object corresponds to a shared resource.
    bool isShared() const { return sharedCount.isSome(); }

    // Validates this Resource_ object.
    Option<Error> validate() const;

    // Check whether this Resource_ object is empty.
    bool isEmpty() const;

    // The `Resource_` arithmetic, comparison operators and `contains()`
    // method require the wrapped `resource` protobuf to have the same
    // sharedness.
    //
    // For shared resources, the `resource` protobuf needs to be equal,
    // and only the shared counters are adjusted or compared.
    // For non-shared resources, the shared counters are none and the
    // semantics of the Resource_ object's operators/contains() method
    // are the same as those of the Resource objects.

    // Checks if this Resource_ is a superset of the given Resource_.
    bool contains(const Resource_& that) const;

    // The arithmetic operators, viz. += and -= assume that the corresponding
    // Resource objects are addable or subtractable already.
    Resource_& operator+=(const Resource_& that);
    Resource_& operator-=(const Resource_& that);

    bool operator==(const Resource_& that) const;
    bool operator!=(const Resource_& that) const;

    // Friend classes and functions for access to private members.
    friend class Resources;
    friend std::ostream& operator<<(
        std::ostream& stream, const Resource_& resource_);

  private:
    // The protobuf Resource that is being managed.
    Resource resource;

    // The counter for grouping shared 'resource' objects, None if the
    // 'resource' is non-shared. This is an int so as to support arithmetic
    // operations involving subtraction.
    Option<int> sharedCount;
  };

public:
  // We rename the type here to alert people about the fact that with
  // `shared_ptr`, no mutation should be made without obtaining exclusive
  // ownership. See `resourcesNoMutationWithoutExclusiveOwnership`.
  using Resource_Unsafe = std::shared_ptr<Resource_>;

  /**
   * Returns a Resource with the given name, value, and role.
   *
   * Parses the text and returns a Resource object with the given name, value,
   * and role. For example, "Resource r = parse("mem", "1024", "*");".
   *
   * @param name The name of the Resource.
   * @param value The Resource's value.
   * @param role The role associated with the Resource.
   * @return A `Try` which contains the parsed Resource if parsing was
   *     successful, or an Error otherwise.
   */
  static Try<Resource> parse(
      const std::string& name,
      const std::string& value,
      const std::string& role);

  /**
   * Parses Resources from an input string.
   *
   * Parses Resources from text in the form of a JSON array or as a simple
   * string in the form of "name(role):value;name:value;...". i.e., this
   * method calls `fromJSON()` or `fromSimpleString()` and validates the
   * resulting `vector<Resource>` before converting it to a `Resources`
   * object.
   *
   * @param text The input string.
   * @param defaultRole The default role.
   * @return A `Try` which contains the parsed Resources if parsing was
   *     successful, or an Error otherwise.
   */
  static Try<Resources> parse(
      const std::string& text,
      const std::string& defaultRole = "*");

  /**
   * Parses an input JSON array into a vector of Resource objects.
   *
   * Parses into a vector of Resource objects from a JSON array. Any
   * resource that doesn't specify a role is assigned to the provided
   * default role. See the `Resource` protobuf definition for precise
   * JSON formatting.
   *
   * Example JSON: [{"name":"cpus","type":"SCALAR","scalar":{"value":8}}]
   *
   * NOTE: The `Resource` objects in the result vector may not be valid
   * semantically (i.e., they may not pass `Resources::validate()`). This
   * is to allow additional handling of the parsing results in some cases.
   *
   * @param resourcesJSON The input JSON array.
   * @param defaultRole The default role.
   * @return A `Try` which contains the parsed vector of Resource objects
   *     if parsing was successful, or an Error otherwise.
   */
  static Try<std::vector<Resource>> fromJSON(
      const JSON::Array& resourcesJSON,
      const std::string& defaultRole = "*");

  /**
   * Parses an input text string into a vector of Resource objects.
   *
   * Parses into a vector of Resource objects from text. Any resource that
   * doesn't specify a role is assigned to the provided default role.
   *
   * Example text: name(role):value;name:value;...
   *
   * NOTE: The `Resource` objects in the result vector may not be valid
   * semantically (i.e., they may not pass `Resources::validate()`). This
   * is to allow additional handling of the parsing results in some cases.
   *
   * @param text The input text string.
   * @param defaultRole The default role.
   * @return A `Try` which contains the parsed vector of Resource objects
   *     if parsing was successful, or an Error otherwise.
   */
  static Try<std::vector<Resource>> fromSimpleString(
      const std::string& text,
      const std::string& defaultRole = "*");

  /**
   * Parse an input string into a vector of Resource objects.
   *
   * Parses into a vector of Resource objects from either JSON or plain
   * text. If the string is well-formed JSON it is assumed to be JSON,
   * otherwise plain text. Any resource that doesn't specify a role is
   * assigned to the provided default role.
   *
   * NOTE: The `Resource` objects in the result vector may not be valid
   * semantically (i.e., they may not pass `Resources::validate()`). This
   * is to allow additional handling of the parsing results in some cases.
   */
  static Try<std::vector<Resource>> fromString(
      const std::string& text,
      const std::string& defaultRole = "*");

  /**
   * Validates a Resource object.
   *
   * Validates the given Resource object. Returns Error if it is not valid. A
   * Resource object is valid if it has a name, a valid type, i.e. scalar,
   * range, or set, has the appropriate value set, and a valid (role,
   * reservation) pair for dynamic reservation.
   *
   * @param resource The input resource to be validated.
   * @return An `Option` which contains None() if the validation was successful,
   *     or an Error if not.
   */
  static Option<Error> validate(const Resource& resource);

  /**
   * Validates the given repeated Resource protobufs.
   *
   * Validates the given repeated Resource protobufs. Returns Error if an
   * invalid Resource is found. A Resource object is valid if it has a name, a
   * valid type, i.e. scalar, range, or set, has the appropriate value set, and
   * a valid (role, reservation) pair for dynamic reservation.
   *
   * TODO(jieyu): Right now, it's the same as checking each individual Resource
   * object in the protobufs. In the future, we could add more checks that are
   * not possible if checking each Resource object individually. For example, we
   * could check multiple usage of an item in a set or a range, etc.
   *
   * @param resources The repeated Resource objects to be validated.
   * @return An `Option` which contains None() if the validation was successful,
   *     or an Error if not.
   */
  static Option<Error> validate(
      const google::protobuf::RepeatedPtrField<Resource>& resources);

  // NOTE: The following predicate functions assume that the given resource is
  // validated, and is in the "post-reservation-refinement" format. That is,
  // the reservation state is represented by `Resource.reservations` field,
  // and `Resource.role` and `Resource.reservation` fields are not set.
  //
  // See 'Resource Format' section in `mesos.proto` for more details.

  // Tests if the given Resource object is empty.
  static bool isEmpty(const Resource& resource);

  // Tests if the given Resource object is a persistent volume.
  static bool isPersistentVolume(const Resource& resource);

  // Tests if the given Resource object is a disk of the specified type.
  static bool isDisk(
      const Resource& resource,
      const Resource::DiskInfo::Source::Type& type);

  // Tests if the given Resource object is reserved. If the role is
  // specified, tests that it's reserved for the given role.
  static bool isReserved(
      const Resource& resource,
      const Option<std::string>& role = None());

  // Tests if the given Resource object is allocatable to the given role.
  // A resource object is allocatable to 'role' if:
  //   * it is reserved to an ancestor of that role in the hierarchy, OR
  //   * it is reserved to 'role' itself, OR
  //   * it is unreserved.
  static bool isAllocatableTo(
      const Resource& resource,
      const std::string& role);

  // Tests if the given Resource object is unreserved.
  static bool isUnreserved(const Resource& resource);

  // Tests if the given Resource object is dynamically reserved.
  static bool isDynamicallyReserved(const Resource& resource);

  // Tests if the given Resource object is revocable.
  static bool isRevocable(const Resource& resource);

  // Tests if the given Resource object is shared.
  static bool isShared(const Resource& resource);

  // Returns true if the resource is allocated to the role subtree
  // (i.e. either to the role itself or to its decedents).
  static bool isAllocatedToRoleSubtree(
      const Resource& resource, const std::string& role);

  // Returns true if the resource is reserved to the role subtree
  // (i.e. either to the role itself or to its decedents).
  static bool isReservedToRoleSubtree(
      const Resource& resource, const std::string& role);

  // Tests if the given Resource object has refined reservations.
  static bool hasRefinedReservations(const Resource& resource);

  // Tests if the given Resource object is provided by a resource provider.
  static bool hasResourceProvider(const Resource& resource);

  // Returns the role to which the given Resource object is reserved for.
  // This must be called only when the resource is reserved!
  static const std::string& reservationRole(const Resource& resource);

  // Shrinks a scalar type `resource` to the target size.
  // Returns true if the resource was shrunk to the target size,
  // or the resource is already within the target size.
  // Returns false otherwise (i.e. the resource is indivisible.
  // E.g. MOUNT volume).
  static bool shrink(Resource* resource, const Value::Scalar& target);

  // Returns the summed up Resources given a hashmap<Key, Resources>.
  //
  // NOTE: While scalar resources such as "cpus" sum correctly,
  // non-scalar resources such as "ports" do not.
  //   e.g. "cpus:2" + "cpus:1" = "cpus:3"
  //        "ports:[0-100]" + "ports:[0-100]" = "ports:[0-100]"
  //
  // TODO(mpark): Deprecate this function once we introduce the
  // concept of "cluster-wide" resources which provides correct
  // semantics for summation over all types of resources. (e.g.
  // non-scalar)
  template <typename Key>
  static Resources sum(const hashmap<Key, Resources>& _resources)
  {
    Resources result;

    foreachvalue (const Resources& resources, _resources) {
      result += resources;
    }

    return result;
  }

  Resources() {}

  // TODO(jieyu): Consider using C++11 initializer list.
  /*implicit*/ Resources(const Resource& resource);
  /*implicit*/ Resources(Resource&& resource);

  /*implicit*/
  Resources(const std::vector<Resource>& _resources);
  Resources(std::vector<Resource>&& _resources);

  /*implicit*/
  Resources(const google::protobuf::RepeatedPtrField<Resource>& _resources);
  Resources(google::protobuf::RepeatedPtrField<Resource>&& _resources);

  Resources(const Resources& that) = default;
  Resources(Resources&& that) = default;

  Resources& operator=(const Resources& that)
  {
    if (this != &that) {
      resourcesNoMutationWithoutExclusiveOwnership =
        that.resourcesNoMutationWithoutExclusiveOwnership;
    }
    return *this;
  }

  Resources& operator=(Resources&& that)
  {
    if (this != &that) {
      resourcesNoMutationWithoutExclusiveOwnership =
        std::move(that.resourcesNoMutationWithoutExclusiveOwnership);
    }
    return *this;
  }

  bool empty() const
  {
    return resourcesNoMutationWithoutExclusiveOwnership.size() == 0;
  }

  size_t size() const
  {
    return resourcesNoMutationWithoutExclusiveOwnership.size();
  }

  // Checks if this Resources is a superset of the given Resources.
  bool contains(const Resources& that) const;

  // Checks if this Resources contains the given Resource.
  bool contains(const Resource& that) const;

  // Checks if the quantities of this `Resources` is a superset of the
  // given `ResourceQuantities`. If a `Resource` object is `SCALAR` type,
  // its quantity is its scalar value. For `RANGES` and `SET` type, their
  // quantities are the number of different instances in the range or set.
  // For example, "range:[1-5]" has a quantity of 5 and "set:{a,b}" has a
  // quantity of 2.
  bool contains(const ResourceQuantities& quantities) const;

  // Count the Resource objects that match the specified value.
  //
  // NOTE:
  // - For a non-shared resource the count can be at most 1 because all
  //   non-shared Resource objects in Resources are unique.
  // - For a shared resource the count can be greater than 1.
  // - If the resource is not in the Resources object, the count is 0.
  size_t count(const Resource& that) const;

  // Allocates the resources to the given role (by setting the
  // `AllocationInfo.role`). Any existing allocation will be
  // over-written.
  void allocate(const std::string& role);

  // Unallocates the resources.
  void unallocate();

  // Filter resources based on the given predicate.
  Resources filter(
      const lambda::function<bool(const Resource&)>& predicate) const;

  // Returns the reserved resources, by role.
  hashmap<std::string, Resources> reservations() const;

  // Returns the reserved resources for the role, if specified.
  // Note that the "*" role represents unreserved resources,
  // and will be ignored.
  Resources reserved(const Option<std::string>& role = None()) const;

  // Returns resources allocatable to role. See `isAllocatableTo` for the
  // definition of 'allocatableTo'.
  Resources allocatableTo(const std::string& role) const;

  // Returns resources that are allocated to the role subtree
  // (i.e. either to the role itself or to its decedents).
  Resources allocatedToRoleSubtree(const std::string& role) const;

  // Returns resources that are reserved to the role subtree
  // (i.e. either to the role itself or to its decedents).
  Resources reservedToRoleSubtree(const std::string& role) const;

  // Returns the unreserved resources.
  Resources unreserved() const;

  // Returns the persistent volumes.
  Resources persistentVolumes() const;

  // Returns the revocable resources.
  Resources revocable() const;

  // Returns the non-revocable resources, effectively !revocable().
  Resources nonRevocable() const;

  // Returns the shared resources.
  Resources shared() const;

  // Returns the non-shared resources.
  Resources nonShared() const;

  // Returns the per-role allocations within these resource objects.
  // This must be called only when the resources are allocated!
  hashmap<std::string, Resources> allocations() const;

  // Returns a `Resources` object with the new reservation added to the back.
  // The new reservation must be a valid refinement of the current reservation.
  Resources pushReservation(const Resource::ReservationInfo& reservation) const;

  // Returns a `Resources` object with the last reservation removed.
  // Every resource in `Resources` must have `resource.reservations_size() > 0`.
  Resources popReservation() const;

  // Returns a `Resources` object with all of the reservations removed.
  Resources toUnreserved() const;

  // Returns a Resources object that contains all the scalar resources
  // but with all the meta-data fields, such as AllocationInfo,
  // ReservationInfo and etc. cleared. Only scalar resources' name,
  // type (SCALAR) and value are preserved.
  //
  // This is intended for code that would like to aggregate together
  // Resource values without regard for metadata like whether the
  // resource is reserved or the particular volume ID in use. For
  // example, when calculating the total resources in a cluster,
  // preserving such information has a major performance cost.
  Resources createStrippedScalarQuantity() const;

  // Finds a Resources object with the same amount of each resource
  // type as "targets" from these Resources. The roles specified in
  // "targets" set the preference order. For each resource type,
  // resources are first taken from the specified role, then from '*',
  // then from any other role.
  // TODO(jieyu): 'find' contains some allocation logic for scalars and
  // fixed set / range elements. However, this is not sufficient for
  // schedulers that want, say, any N available ports. We should
  // consider moving this to an internal "allocation" library for our
  // example frameworks to leverage.
  Option<Resources> find(const Resources& targets) const;

  // Applies a resource conversion by taking out the `consumed`
  // resources and adding back the `converted` resources. Returns an
  // Error if the conversion cannot be applied.
  Try<Resources> apply(const ResourceConversion& conversion) const;

  // Finds a resource object with the same metadata (i.e. AllocationInfo,
  // ReservationInfo, etc.) as the given one, ignoring the actual value.
  // If multiple matching resources exist, the first match is returned.
  Option<Resource> match(const Resource& resource) const;

  // Obtains the conversion from the given operation and applies the
  // conversion. This method serves a syntax sugar for applying a
  // resource conversion.
  // TODO(jieyu): Consider remove this method once we updated all the
  // call sites.
  Try<Resources> apply(const Offer::Operation& operation) const;

  template <typename Iterable>
  Try<Resources> apply(const Iterable& iterable) const
  {
    Resources result = *this;

    foreach (const auto& t, iterable) {
      Try<Resources> converted = result.apply(t);
      if (converted.isError()) {
        return Error(converted.error());
      }

      result = converted.get();
    }

    return result;
  }

  // Helpers to get resource values. We consider all roles here.
  template <typename T>
  Option<T> get(const std::string& name) const;

  // Get resources of the given name.
  Resources get(const std::string& name) const;

  // Get all the resources that are scalars.
  Resources scalars() const;

  // Get the set of unique resource names.
  std::set<std::string> names() const;

  // Get the types of resources associated with each resource name.
  // NOTE: Resources of the same name must have the same type, as
  // enforced by Resources::parse().
  std::map<std::string, Value_Type> types() const;

  // Helpers to get known resource types.
  // TODO(vinod): Fix this when we make these types as first class
  // protobufs.
  Option<double> cpus() const;
  Option<double> gpus() const;
  Option<Bytes> mem() const;
  Option<Bytes> disk() const;

  // TODO(vinod): Provide a Ranges abstraction.
  Option<Value::Ranges> ports() const;

  // TODO(jieyu): Consider returning an EphemeralPorts abstraction
  // which holds the ephemeral ports allocation logic.
  Option<Value::Ranges> ephemeral_ports() const;

  // We use `boost::indirect_iterator` to expose `Resource` (implicitly
  // converted from `Resource_`) iteration, while actually storing
  // `Resource_Unsafe`.
  //
  // NOTE: Non-const `begin()` and `end()` intentionally return const
  // iterators to prevent mutable access to the `Resource` objects.

  typedef boost::indirect_iterator<
      std::vector<Resource_Unsafe>::const_iterator>
    const_iterator;

  const_iterator begin()
  {
    return static_cast<const std::vector<Resource_Unsafe>&>(
               resourcesNoMutationWithoutExclusiveOwnership)
      .begin();
  }

  const_iterator end()
  {
    return static_cast<const std::vector<Resource_Unsafe>&>(
               resourcesNoMutationWithoutExclusiveOwnership)
      .end();
  }

  const_iterator begin() const
  {
    return resourcesNoMutationWithoutExclusiveOwnership.begin();
  }

  const_iterator end() const
  {
    return resourcesNoMutationWithoutExclusiveOwnership.end();
  }

  // Using this operator makes it easy to copy a resources object into
  // a protocol buffer field.
  // Note that the google::protobuf::RepeatedPtrField<Resource> is
  // generated at runtime.
  operator google::protobuf::RepeatedPtrField<Resource>() const;

  bool operator==(const Resources& that) const;
  bool operator!=(const Resources& that) const;

  // NOTE: If any error occurs (e.g., input Resource is not valid or
  // the first operand is not a superset of the second operand while
  // doing subtraction), the semantics is as though the second operand
  // was actually just an empty resource (as though you didn't do the
  // operation at all).
  Resources operator+(const Resource& that) const &;
  Resources operator+(const Resource& that) &&;

  Resources operator+(Resource&& that) const &;
  Resources operator+(Resource&& that) &&;

  Resources& operator+=(const Resource& that);
  Resources& operator+=(Resource&& that);

  Resources operator+(const Resources& that) const &;
  Resources operator+(const Resources& that) &&;

  Resources operator+(Resources&& that) const &;
  Resources operator+(Resources&& that) &&;

  Resources& operator+=(const Resources& that);
  Resources& operator+=(Resources&& that);

  Resources operator-(const Resource& that) const;
  Resources operator-(const Resources& that) const;
  Resources& operator-=(const Resource& that);
  Resources& operator-=(const Resources& that);

  friend std::ostream& operator<<(
      std::ostream& stream, const Resource_& resource_);

private:
  // Similar to 'contains(const Resource&)' but skips the validity
  // check. This can be used to avoid the performance overhead of
  // calling 'contains(const Resource&)' when the resource can be
  // assumed valid (e.g. it's inside a Resources).
  //
  // TODO(jieyu): Measure performance overhead of validity check to
  // ensure this is warranted.
  bool _contains(const Resource_& that) const;

  // Similar to the public 'find', but only for a single Resource
  // object. The target resource may span multiple roles, so this
  // returns Resources.
  Option<Resources> find(const Resource& target) const;

  // Validation-free versions of += and -= `Resource_` operators.
  // These can be used when `r` is already validated.
  //
  // NOTE: `Resource` objects are implicitly converted to `Resource_`
  // objects, so here the API can also accept a `Resource` object.
  void add(const Resource_& r);
  void add(Resource_&& r);

  // TODO(mzhu): Add move support.
  void add(const Resource_Unsafe& that);

  void subtract(const Resource_& r);

  Resources& operator+=(const Resource_& that);
  Resources& operator+=(Resource_&& that);

  Resources& operator-=(const Resource_& that);

  // Resources are stored using copy-on-write:
  //
  //   (1) Copies are done by copying the `shared_ptr`. This
  //       makes read-only filtering (e.g. `unreserved()`)
  //       inexpensive as we do not have to perform copies
  //       of the resource objects.
  //
  //   (2) When a write occurs:
  //      (a) If there's a single reference to the resource
  //          object, we mutate directly.
  //      (b) If there's more than a single reference to the
  //          resource object, we copy first, then mutate the copy.
  //
  // We name the `vector` field `resourcesNoMutationWithoutExclusiveOwnership`
  // and typedef its item type to `Resource_Unsafe` to alert people
  // regarding (2).
  //
  // TODO(mzhu): While naming the vector and its item type may help, this is
  // still brittle and certainly not ideal. Explore more robust designs such as
  // introducing a customized copy-on-write abstraction that hides direct
  // setters and only allow mutations in a controlled fashion.
  //
  // TODO(mzhu): Consider using `boost::intrusive_ptr` for
  // possibly better performance.
  std::vector<Resource_Unsafe> resourcesNoMutationWithoutExclusiveOwnership;
};


std::ostream& operator<<(
    std::ostream& stream,
    const Resources::Resource_& resource);


std::ostream& operator<<(std::ostream& stream, const Resource& resource);


std::ostream& operator<<(std::ostream& stream, const Resources& resources);


std::ostream& operator<<(
    std::ostream& stream,
    const google::protobuf::RepeatedPtrField<Resource>& resources);


inline Resources operator+(
    const google::protobuf::RepeatedPtrField<Resource>& left,
    const Resources& right)
{
  return Resources(left) + right;
}


inline Resources operator-(
    const google::protobuf::RepeatedPtrField<Resource>& left,
    const Resources& right)
{
  return Resources(left) - right;
}


inline bool operator==(
    const google::protobuf::RepeatedPtrField<Resource>& left,
    const Resources& right)
{
  return Resources(left) == right;
}


template <typename Key>
hashmap<Key, Resources>& operator+=(
    hashmap<Key, Resources>& left,
    const hashmap<Key, Resources>& right)
{
  foreachpair (const Key& key, const Resources& resources, right) {
    left[key] += resources;
  }
  return left;
}


template <typename Key>
hashmap<Key, Resources> operator+(
    const hashmap<Key, Resources>& left,
    const hashmap<Key, Resources>& right)
{
  hashmap<Key, Resources> result = left;
  result += right;
  return result;
}


// Tests if `right` is contained in `left`, note that most
// callers should just make use of `Resources::contains(...)`.
// However, if dealing only with singular `Resource` objects,
// this has lower overhead.
//
// NOTE: `left` and `right` must be valid resource objects.
bool contains(const Resource& left, const Resource& right);


/**
 * Represents a resource conversion, usually as a result of an offer
 * operation. See more details in `Resources::apply` method.
 */
class ResourceConversion
{
public:
  typedef lambda::function<Try<Nothing>(const Resources&)> PostValidation;

  ResourceConversion(
      const Resources& _consumed,
      const Resources& _converted,
      const Option<PostValidation>& _postValidation = None())
    : consumed(_consumed),
      converted(_converted),
      postValidation(_postValidation) {}

  Try<Resources> apply(const Resources& resources) const;

  Resources consumed;
  Resources converted;
  Option<PostValidation> postValidation;
};

} // namespace mesos {

#endif // __RESOURCES_HPP__