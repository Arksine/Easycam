/*
 * Copyright (C) 2011-2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * This file is auto-generated. DO NOT MODIFY!
 * The source Renderscript file: convert.rs
 */

#include "RenderScript.h"

using namespace android::RSC;

/* This class encapsulates access to the exported elements of the script.  Typically, you
 * would instantiate this class once, call the set_* methods for each of the exported global
 * variables you want to change, then call one of the forEach_ methods to invoke a kernel.
 */
class ScriptC_convert : public android::RSC::ScriptC {
private:
    /* For each non-const variable exported by the script, we have an equivalent field.  This
     * field contains the last value this variable was set to using the set_ method.  This may
     * not be current value of the variable in the script, as the script is free to modify its
     * internal variable without changing this field.  If the script initializes the
     * exported variable, the constructor will initialize this field to the same value.
     */
    android::RSC::sp<const android::RSC::Allocation> mExportVar_output;
    int32_t mExportVar_xElements;
    /* The following elements are used to verify the types of allocations passed to kernels.
     */
    android::RSC::sp<const android::RSC::Element> __rs_elem_U8_4;
public:
    ScriptC_convert(android::RSC::sp<android::RSC::RS> rs);
    virtual ~ScriptC_convert();

    /* Methods to set and get the variables exported by the script. Const variables will not
     * have a setter.
     * 
     * Note that the value returned by the getter may not be the current value of the variable
     * in the script.  The getter will return the initial value of the variable (as defined in
     * the script) or the the last value set by using the setter method.  The script is free to
     * modify its value independently.
     */
    void set_output(android::RSC::sp<const android::RSC::Allocation> v) {
        setVar(0, v);
        mExportVar_output = v;
    }

    android::RSC::sp<const android::RSC::Allocation> get_output() const {
        return mExportVar_output;
    }

    void set_xElements(int32_t v) {
        setVar(1, &v, sizeof(v));
        mExportVar_xElements = v;
    }

    int32_t get_xElements() const {
        return mExportVar_xElements;
    }

    // No forEach_root(...)
    /* For each kernel of the script corresponds one method.  That method queues the kernel
     * for execution.  The kernel may not have completed nor even started by the time this
     * function returns.  Calls that extract the data out of the output allocation will wait
     * for the kernels to complete.