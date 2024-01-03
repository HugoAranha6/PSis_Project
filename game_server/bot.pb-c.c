/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: bot.proto */

/* Do not generate deprecated warnings for self */
#ifndef PROTOBUF_C__NO_DEPRECATED
#define PROTOBUF_C__NO_DEPRECATED
#endif

#include "bot.pb-c.h"
void   bot_connect__init
                     (BotConnect         *message)
{
  static const BotConnect init_value = BOT_CONNECT__INIT;
  *message = init_value;
}
size_t bot_connect__get_packed_size
                     (const BotConnect *message)
{
  assert(message->base.descriptor == &bot_connect__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t bot_connect__pack
                     (const BotConnect *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &bot_connect__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t bot_connect__pack_to_buffer
                     (const BotConnect *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &bot_connect__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
BotConnect *
       bot_connect__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (BotConnect *)
     protobuf_c_message_unpack (&bot_connect__descriptor,
                                allocator, len, data);
}
void   bot_connect__free_unpacked
                     (BotConnect *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &bot_connect__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   bot_movement__init
                     (BotMovement         *message)
{
  static const BotMovement init_value = BOT_MOVEMENT__INIT;
  *message = init_value;
}
size_t bot_movement__get_packed_size
                     (const BotMovement *message)
{
  assert(message->base.descriptor == &bot_movement__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t bot_movement__pack
                     (const BotMovement *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &bot_movement__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t bot_movement__pack_to_buffer
                     (const BotMovement *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &bot_movement__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
BotMovement *
       bot_movement__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (BotMovement *)
     protobuf_c_message_unpack (&bot_movement__descriptor,
                                allocator, len, data);
}
void   bot_movement__free_unpacked
                     (BotMovement *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &bot_movement__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   bot_disconnect__init
                     (BotDisconnect         *message)
{
  static const BotDisconnect init_value = BOT_DISCONNECT__INIT;
  *message = init_value;
}
size_t bot_disconnect__get_packed_size
                     (const BotDisconnect *message)
{
  assert(message->base.descriptor == &bot_disconnect__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t bot_disconnect__pack
                     (const BotDisconnect *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &bot_disconnect__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t bot_disconnect__pack_to_buffer
                     (const BotDisconnect *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &bot_disconnect__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
BotDisconnect *
       bot_disconnect__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (BotDisconnect *)
     protobuf_c_message_unpack (&bot_disconnect__descriptor,
                                allocator, len, data);
}
void   bot_disconnect__free_unpacked
                     (BotDisconnect *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &bot_disconnect__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   connect_repply__init
                     (ConnectRepply         *message)
{
  static const ConnectRepply init_value = CONNECT_REPPLY__INIT;
  *message = init_value;
}
size_t connect_repply__get_packed_size
                     (const ConnectRepply *message)
{
  assert(message->base.descriptor == &connect_repply__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t connect_repply__pack
                     (const ConnectRepply *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &connect_repply__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t connect_repply__pack_to_buffer
                     (const ConnectRepply *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &connect_repply__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
ConnectRepply *
       connect_repply__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (ConnectRepply *)
     protobuf_c_message_unpack (&connect_repply__descriptor,
                                allocator, len, data);
}
void   connect_repply__free_unpacked
                     (ConnectRepply *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &connect_repply__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
static const ProtobufCFieldDescriptor bot_connect__field_descriptors[1] =
{
  {
    "score",
    1,
    PROTOBUF_C_LABEL_REPEATED,
    PROTOBUF_C_TYPE_INT32,
    offsetof(BotConnect, n_score),
    offsetof(BotConnect, score),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned bot_connect__field_indices_by_name[] = {
  0,   /* field[0] = score */
};
static const ProtobufCIntRange bot_connect__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 1 }
};
const ProtobufCMessageDescriptor bot_connect__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "bot_connect",
  "BotConnect",
  "BotConnect",
  "",
  sizeof(BotConnect),
  1,
  bot_connect__field_descriptors,
  bot_connect__field_indices_by_name,
  1,  bot_connect__number_ranges,
  (ProtobufCMessageInit) bot_connect__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCFieldDescriptor bot_movement__field_descriptors[3] =
{
  {
    "id",
    1,
    PROTOBUF_C_LABEL_REPEATED,
    PROTOBUF_C_TYPE_INT32,
    offsetof(BotMovement, n_id),
    offsetof(BotMovement, id),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "token",
    2,
    PROTOBUF_C_LABEL_REPEATED,
    PROTOBUF_C_TYPE_INT32,
    offsetof(BotMovement, n_token),
    offsetof(BotMovement, token),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "movement",
    3,
    PROTOBUF_C_LABEL_REPEATED,
    PROTOBUF_C_TYPE_ENUM,
    offsetof(BotMovement, n_movement),
    offsetof(BotMovement, movement),
    &direction_move__descriptor,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned bot_movement__field_indices_by_name[] = {
  0,   /* field[0] = id */
  2,   /* field[2] = movement */
  1,   /* field[1] = token */
};
static const ProtobufCIntRange bot_movement__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 3 }
};
const ProtobufCMessageDescriptor bot_movement__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "bot_movement",
  "BotMovement",
  "BotMovement",
  "",
  sizeof(BotMovement),
  3,
  bot_movement__field_descriptors,
  bot_movement__field_indices_by_name,
  1,  bot_movement__number_ranges,
  (ProtobufCMessageInit) bot_movement__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCFieldDescriptor bot_disconnect__field_descriptors[2] =
{
  {
    "id",
    1,
    PROTOBUF_C_LABEL_REPEATED,
    PROTOBUF_C_TYPE_INT32,
    offsetof(BotDisconnect, n_id),
    offsetof(BotDisconnect, id),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "token",
    2,
    PROTOBUF_C_LABEL_REPEATED,
    PROTOBUF_C_TYPE_INT32,
    offsetof(BotDisconnect, n_token),
    offsetof(BotDisconnect, token),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned bot_disconnect__field_indices_by_name[] = {
  0,   /* field[0] = id */
  1,   /* field[1] = token */
};
static const ProtobufCIntRange bot_disconnect__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 2 }
};
const ProtobufCMessageDescriptor bot_disconnect__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "bot_disconnect",
  "BotDisconnect",
  "BotDisconnect",
  "",
  sizeof(BotDisconnect),
  2,
  bot_disconnect__field_descriptors,
  bot_disconnect__field_indices_by_name,
  1,  bot_disconnect__number_ranges,
  (ProtobufCMessageInit) bot_disconnect__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCFieldDescriptor connect_repply__field_descriptors[2] =
{
  {
    "id",
    1,
    PROTOBUF_C_LABEL_REPEATED,
    PROTOBUF_C_TYPE_INT32,
    offsetof(ConnectRepply, n_id),
    offsetof(ConnectRepply, id),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "token",
    2,
    PROTOBUF_C_LABEL_REPEATED,
    PROTOBUF_C_TYPE_INT32,
    offsetof(ConnectRepply, n_token),
    offsetof(ConnectRepply, token),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned connect_repply__field_indices_by_name[] = {
  0,   /* field[0] = id */
  1,   /* field[1] = token */
};
static const ProtobufCIntRange connect_repply__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 2 }
};
const ProtobufCMessageDescriptor connect_repply__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "connect_repply",
  "ConnectRepply",
  "ConnectRepply",
  "",
  sizeof(ConnectRepply),
  2,
  connect_repply__field_descriptors,
  connect_repply__field_indices_by_name,
  1,  connect_repply__number_ranges,
  (ProtobufCMessageInit) connect_repply__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCEnumValue direction_move__enum_values_by_number[5] =
{
  { "UP", "DIRECTION_MOVE__UP", 0 },
  { "DOWN", "DIRECTION_MOVE__DOWN", 1 },
  { "LEFT", "DIRECTION_MOVE__LEFT", 2 },
  { "RIGHT", "DIRECTION_MOVE__RIGHT", 3 },
  { "HOLD", "DIRECTION_MOVE__HOLD", 4 },
};
static const ProtobufCIntRange direction_move__value_ranges[] = {
{0, 0},{0, 5}
};
static const ProtobufCEnumValueIndex direction_move__enum_values_by_name[5] =
{
  { "DOWN", 1 },
  { "HOLD", 4 },
  { "LEFT", 2 },
  { "RIGHT", 3 },
  { "UP", 0 },
};
const ProtobufCEnumDescriptor direction_move__descriptor =
{
  PROTOBUF_C__ENUM_DESCRIPTOR_MAGIC,
  "direction_move",
  "direction_move",
  "DirectionMove",
  "",
  5,
  direction_move__enum_values_by_number,
  5,
  direction_move__enum_values_by_name,
  1,
  direction_move__value_ranges,
  NULL,NULL,NULL,NULL   /* reserved[1234] */
};
