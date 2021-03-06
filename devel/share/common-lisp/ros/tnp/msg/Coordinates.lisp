; Auto-generated. Do not edit!


(cl:in-package tnp-msg)


;//! \htmlinclude Coordinates.msg.html

(cl:defclass <Coordinates> (roslisp-msg-protocol:ros-message)
  ((placemark_type
    :reader placemark_type
    :initarg :placemark_type
    :type cl:string
    :initform "")
   (longitude
    :reader longitude
    :initarg :longitude
    :type (cl:vector cl:float)
   :initform (cl:make-array 0 :element-type 'cl:float :initial-element 0.0))
   (latitude
    :reader latitude
    :initarg :latitude
    :type (cl:vector cl:float)
   :initform (cl:make-array 0 :element-type 'cl:float :initial-element 0.0)))
)

(cl:defclass Coordinates (<Coordinates>)
  ())

(cl:defmethod cl:initialize-instance :after ((m <Coordinates>) cl:&rest args)
  (cl:declare (cl:ignorable args))
  (cl:unless (cl:typep m 'Coordinates)
    (roslisp-msg-protocol:msg-deprecation-warning "using old message class name tnp-msg:<Coordinates> is deprecated: use tnp-msg:Coordinates instead.")))

(cl:ensure-generic-function 'placemark_type-val :lambda-list '(m))
(cl:defmethod placemark_type-val ((m <Coordinates>))
  (roslisp-msg-protocol:msg-deprecation-warning "Using old-style slot reader tnp-msg:placemark_type-val is deprecated.  Use tnp-msg:placemark_type instead.")
  (placemark_type m))

(cl:ensure-generic-function 'longitude-val :lambda-list '(m))
(cl:defmethod longitude-val ((m <Coordinates>))
  (roslisp-msg-protocol:msg-deprecation-warning "Using old-style slot reader tnp-msg:longitude-val is deprecated.  Use tnp-msg:longitude instead.")
  (longitude m))

(cl:ensure-generic-function 'latitude-val :lambda-list '(m))
(cl:defmethod latitude-val ((m <Coordinates>))
  (roslisp-msg-protocol:msg-deprecation-warning "Using old-style slot reader tnp-msg:latitude-val is deprecated.  Use tnp-msg:latitude instead.")
  (latitude m))
(cl:defmethod roslisp-msg-protocol:serialize ((msg <Coordinates>) ostream)
  "Serializes a message object of type '<Coordinates>"
  (cl:let ((__ros_str_len (cl:length (cl:slot-value msg 'placemark_type))))
    (cl:write-byte (cl:ldb (cl:byte 8 0) __ros_str_len) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 8) __ros_str_len) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 16) __ros_str_len) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 24) __ros_str_len) ostream))
  (cl:map cl:nil #'(cl:lambda (c) (cl:write-byte (cl:char-code c) ostream)) (cl:slot-value msg 'placemark_type))
  (cl:let ((__ros_arr_len (cl:length (cl:slot-value msg 'longitude))))
    (cl:write-byte (cl:ldb (cl:byte 8 0) __ros_arr_len) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 8) __ros_arr_len) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 16) __ros_arr_len) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 24) __ros_arr_len) ostream))
  (cl:map cl:nil #'(cl:lambda (ele) (cl:let ((bits (roslisp-utils:encode-double-float-bits ele)))
    (cl:write-byte (cl:ldb (cl:byte 8 0) bits) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 8) bits) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 16) bits) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 24) bits) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 32) bits) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 40) bits) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 48) bits) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 56) bits) ostream)))
   (cl:slot-value msg 'longitude))
  (cl:let ((__ros_arr_len (cl:length (cl:slot-value msg 'latitude))))
    (cl:write-byte (cl:ldb (cl:byte 8 0) __ros_arr_len) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 8) __ros_arr_len) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 16) __ros_arr_len) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 24) __ros_arr_len) ostream))
  (cl:map cl:nil #'(cl:lambda (ele) (cl:let ((bits (roslisp-utils:encode-double-float-bits ele)))
    (cl:write-byte (cl:ldb (cl:byte 8 0) bits) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 8) bits) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 16) bits) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 24) bits) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 32) bits) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 40) bits) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 48) bits) ostream)
    (cl:write-byte (cl:ldb (cl:byte 8 56) bits) ostream)))
   (cl:slot-value msg 'latitude))
)
(cl:defmethod roslisp-msg-protocol:deserialize ((msg <Coordinates>) istream)
  "Deserializes a message object of type '<Coordinates>"
    (cl:let ((__ros_str_len 0))
      (cl:setf (cl:ldb (cl:byte 8 0) __ros_str_len) (cl:read-byte istream))
      (cl:setf (cl:ldb (cl:byte 8 8) __ros_str_len) (cl:read-byte istream))
      (cl:setf (cl:ldb (cl:byte 8 16) __ros_str_len) (cl:read-byte istream))
      (cl:setf (cl:ldb (cl:byte 8 24) __ros_str_len) (cl:read-byte istream))
      (cl:setf (cl:slot-value msg 'placemark_type) (cl:make-string __ros_str_len))
      (cl:dotimes (__ros_str_idx __ros_str_len msg)
        (cl:setf (cl:char (cl:slot-value msg 'placemark_type) __ros_str_idx) (cl:code-char (cl:read-byte istream)))))
  (cl:let ((__ros_arr_len 0))
    (cl:setf (cl:ldb (cl:byte 8 0) __ros_arr_len) (cl:read-byte istream))
    (cl:setf (cl:ldb (cl:byte 8 8) __ros_arr_len) (cl:read-byte istream))
    (cl:setf (cl:ldb (cl:byte 8 16) __ros_arr_len) (cl:read-byte istream))
    (cl:setf (cl:ldb (cl:byte 8 24) __ros_arr_len) (cl:read-byte istream))
  (cl:setf (cl:slot-value msg 'longitude) (cl:make-array __ros_arr_len))
  (cl:let ((vals (cl:slot-value msg 'longitude)))
    (cl:dotimes (i __ros_arr_len)
    (cl:let ((bits 0))
      (cl:setf (cl:ldb (cl:byte 8 0) bits) (cl:read-byte istream))
      (cl:setf (cl:ldb (cl:byte 8 8) bits) (cl:read-byte istream))
      (cl:setf (cl:ldb (cl:byte 8 16) bits) (cl:read-byte istream))
      (cl:setf (cl:ldb (cl:byte 8 24) bits) (cl:read-byte istream))
      (cl:setf (cl:ldb (cl:byte 8 32) bits) (cl:read-byte istream))
      (cl:setf (cl:ldb (cl:byte 8 40) bits) (cl:read-byte istream))
      (cl:setf (cl:ldb (cl:byte 8 48) bits) (cl:read-byte istream))
      (cl:setf (cl:ldb (cl:byte 8 56) bits) (cl:read-byte istream))
    (cl:setf (cl:aref vals i) (roslisp-utils:decode-double-float-bits bits))))))
  (cl:let ((__ros_arr_len 0))
    (cl:setf (cl:ldb (cl:byte 8 0) __ros_arr_len) (cl:read-byte istream))
    (cl:setf (cl:ldb (cl:byte 8 8) __ros_arr_len) (cl:read-byte istream))
    (cl:setf (cl:ldb (cl:byte 8 16) __ros_arr_len) (cl:read-byte istream))
    (cl:setf (cl:ldb (cl:byte 8 24) __ros_arr_len) (cl:read-byte istream))
  (cl:setf (cl:slot-value msg 'latitude) (cl:make-array __ros_arr_len))
  (cl:let ((vals (cl:slot-value msg 'latitude)))
    (cl:dotimes (i __ros_arr_len)
    (cl:let ((bits 0))
      (cl:setf (cl:ldb (cl:byte 8 0) bits) (cl:read-byte istream))
      (cl:setf (cl:ldb (cl:byte 8 8) bits) (cl:read-byte istream))
      (cl:setf (cl:ldb (cl:byte 8 16) bits) (cl:read-byte istream))
      (cl:setf (cl:ldb (cl:byte 8 24) bits) (cl:read-byte istream))
      (cl:setf (cl:ldb (cl:byte 8 32) bits) (cl:read-byte istream))
      (cl:setf (cl:ldb (cl:byte 8 40) bits) (cl:read-byte istream))
      (cl:setf (cl:ldb (cl:byte 8 48) bits) (cl:read-byte istream))
      (cl:setf (cl:ldb (cl:byte 8 56) bits) (cl:read-byte istream))
    (cl:setf (cl:aref vals i) (roslisp-utils:decode-double-float-bits bits))))))
  msg
)
(cl:defmethod roslisp-msg-protocol:ros-datatype ((msg (cl:eql '<Coordinates>)))
  "Returns string type for a message object of type '<Coordinates>"
  "tnp/Coordinates")
(cl:defmethod roslisp-msg-protocol:ros-datatype ((msg (cl:eql 'Coordinates)))
  "Returns string type for a message object of type 'Coordinates"
  "tnp/Coordinates")
(cl:defmethod roslisp-msg-protocol:md5sum ((type (cl:eql '<Coordinates>)))
  "Returns md5sum for a message object of type '<Coordinates>"
  "5233f15c788dc5823ec41234899f903f")
(cl:defmethod roslisp-msg-protocol:md5sum ((type (cl:eql 'Coordinates)))
  "Returns md5sum for a message object of type 'Coordinates"
  "5233f15c788dc5823ec41234899f903f")
(cl:defmethod roslisp-msg-protocol:message-definition ((type (cl:eql '<Coordinates>)))
  "Returns full string definition for message of type '<Coordinates>"
  (cl:format cl:nil "string placemark_type~%float64[] longitude~%float64[] latitude~%~%~%"))
(cl:defmethod roslisp-msg-protocol:message-definition ((type (cl:eql 'Coordinates)))
  "Returns full string definition for message of type 'Coordinates"
  (cl:format cl:nil "string placemark_type~%float64[] longitude~%float64[] latitude~%~%~%"))
(cl:defmethod roslisp-msg-protocol:serialization-length ((msg <Coordinates>))
  (cl:+ 0
     4 (cl:length (cl:slot-value msg 'placemark_type))
     4 (cl:reduce #'cl:+ (cl:slot-value msg 'longitude) :key #'(cl:lambda (ele) (cl:declare (cl:ignorable ele)) (cl:+ 8)))
     4 (cl:reduce #'cl:+ (cl:slot-value msg 'latitude) :key #'(cl:lambda (ele) (cl:declare (cl:ignorable ele)) (cl:+ 8)))
))
(cl:defmethod roslisp-msg-protocol:ros-message-to-list ((msg <Coordinates>))
  "Converts a ROS message object to a list"
  (cl:list 'Coordinates
    (cl:cons ':placemark_type (placemark_type msg))
    (cl:cons ':longitude (longitude msg))
    (cl:cons ':latitude (latitude msg))
))
