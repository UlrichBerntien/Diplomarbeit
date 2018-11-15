/* entstanden aus
**     ISERRS.C -- DT-IRIS (SP0217) C error definitions
\************************************************************************/

enum {
  ise_normal             = 0 ,       /* No error; success!           */
  ise_argument           = 100 ,     /* Argument error.              */
  ise_not_initialized    = 101 ,     /* DT-IRIS subroutines not
                                        properly initialized.        */
  ise_init_done          = 102 ,     /* Initialization has already
                                        been done.                   */
  ise_no_internal_memory = 103 ,     /* Initialization aborted; not
                                        enough memory for internal
                                        use.                         */
  ise_frame_table_full   = 104 ,     /* No more frame buffers
                                        available.                   */
  ise_no_memory          = 105 ,     /* Insufficient memory for
                                        buffer allocation.           */
  ise_buffer_exist       = 106 ,     /* Frame buffer already exist.  */
  ise_buffer_not_found   = 107 ,     /* Unknown frame buffer.        */
  ise_bad_memory         = 108 ,     /* Memory corruption error --
                                        suggest reboot.              */
  ise_file_create        = 109 ,     /* File could not be opened for
                                        writing.                     */
  ise_file_write         = 110 ,     /* Error while writing file.    */
  ise_file_open          = 111 ,     /* File could not be opened for
                                        reading.                     */
  ise_file_read          = 112 ,     /* Error while reading file.    */
  ise_invalid_image_file = 113 ,     /* File was not recognized as a
                                        compatible image file.       */
  ise_no_driver          = 114 ,     /* Initialization aborted;
                                        DT-IRIS device driver not
                                        installed.                   */
  ise_many_channels      = 115 ,     /* Initialization aborted; too
                                        many file channels open
                                        already.                     */
  ise_many_files         = 116 ,     /* Too many files open already. */
  ise_bad_config         = 117 ,     /* Device configuration did not
                                        verify; run DT-IRIS SET-UP.  */
  ise_device             = 118 ,     /* Device error.                */
  ise_no2853             = 119 ,     /* Device driver could not
                                        locate a DT2853 device.      */
  ise_res120             = 120 ,     /* Reserved.  Report this error
                                        to Data Translation.         */
  ise_no2859             = 121 ,     /* Device driver could not
                                        locate a DT2859 device.      */
  ise_external_sync      = 122 ,     /* External sync should be used
                                        for acquisition or passthru. */
  ise_timeout            = 123 ,     /* Device timed out.            */
  ise_vertical_sync      = 124 ,     /* External sync source is not
                                        compatible.                  */
  ise_mamory_failure     = 125 ,     /* Bad memory location detected
                                        in the device frame-store.   */
  ise_res126             = 126 ,     /* Reserved.                    */
  ise_trigacq_wait       = 127 ,     /* Triggered acquisition in
                                        progress; attempted operation
                                        could not be done.           */
  ise_res128             = 128 ,     /* Reserved.                    */
  ise_res129             = 129 ,     /* Reserved.                    */
  ise_no_subroutine      = 130 ,     /* Call made to a non-existent
                                        subroutine.                  */
  ise_character_not_found = 131 ,    /* Character index not part of
                                        current selected font.       */
  ise_font_exist         = 132 ,     /* Font already exist.          */
  ise_font_not_found     = 133 ,     /* Font does not exist.         */
  ise_radius_too_large   = 134 ,     /* Radius of circle is too
                                        large.                       */
  ise_subroutine_unimplemented  = 135 ,    /* DT-IRIS subroutine is
                                              currently not implemented. */
  ise_invalid_driver     = 136 ,     /* DT-IRIS device driver
                                        reported an unknown error.   */
  ise_size_limit         = 137 ,     /* REGION/PIXEL buffer operation
                                        size to large                */
  ise_rev_mismatch       = 138 };    /* DT-IRIS driver/subroutine
                                        revision mismatch            */

