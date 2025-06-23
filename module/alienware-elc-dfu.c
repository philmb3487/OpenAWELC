#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <uapi/linux/stat.h> /* S_IRUSR, S_IWUSR  */
#include <linux/acpi.h>
#include <linux/delay.h>

#define WMAX_CONTROL_GUID		"A70591CE-A997-11DA-B012-B622A1EF5492"

struct wmax_u32_args {
    u32 arg;
};

static acpi_status alienware_wmax_command(void *in_args, size_t in_size,
                                          u32 command, u32 *out_data)
{
    acpi_status status;
    union acpi_object *obj;
    struct acpi_buffer input;
    struct acpi_buffer output;

    input.length = in_size;
    input.pointer = in_args;
    if (out_data) {
        output.length = ACPI_ALLOCATE_BUFFER;
        output.pointer = NULL;
        status = wmi_evaluate_method(WMAX_CONTROL_GUID, 0,
                                     command, &input, &output);
        if (ACPI_SUCCESS(status)) {
            obj = (union acpi_object *)output.pointer;
            if (obj && obj->type == ACPI_TYPE_INTEGER)
                *out_data = (u32)obj->integer.value;
        }
        kfree(output.pointer);
    } else {
        status = wmi_evaluate_method(WMAX_CONTROL_GUID, 0,
                                     command, &input, NULL);
    }
    return status;
}

/*
 * Reset the ELC into DFU (programming) mode.
 */
static void alienware_stm32_dfu()
{
    struct wmax_u32_args in_args = {
        .arg = 0,
    };
    acpi_status status;
    u32 out_data;
    const unsigned WMAX_METHOD_FWUPDATE_GPIO = 32;

    in_args.arg = 1;
    status = alienware_wmax_command(&in_args, sizeof(in_args),
                                    WMAX_METHOD_FWUPDATE_GPIO, &out_data);
    if (!ACPI_SUCCESS(status)) {
        goto err;
    }
    msleep(50);
    in_args.arg = 256;
    status = alienware_wmax_command(&in_args, sizeof(in_args),
                                    WMAX_METHOD_FWUPDATE_GPIO, &out_data);
    if (!ACPI_SUCCESS(status)) {
        goto err;
    }
    msleep(50);
    in_args.arg = 257;
    status = alienware_wmax_command(&in_args, sizeof(in_args),
                                    WMAX_METHOD_FWUPDATE_GPIO, &out_data);
    if (ACPI_SUCCESS(status)) {
        if (out_data == 0) {
            pr_warn("alienware-wmi: stm32 in dfu mode. Please be careful.\n");
            return;
        }
    } else {
        goto err;
    }

    err:
    pr_err("alienware-wmi: unknown dfu status: %d\n", status);
}

/*
 * Boot the ELC.
 */
static void alienware_stm32_run()
{
    struct wmax_u32_args in_args = {
        .arg = 0,
    };
    acpi_status status;
    u32 out_data;
    const unsigned WMAX_METHOD_FWUPDATE_GPIO = 32;

    in_args.arg = 1;
    status = alienware_wmax_command(&in_args, sizeof(in_args),
                                    WMAX_METHOD_FWUPDATE_GPIO, &out_data);
    if (!ACPI_SUCCESS(status)) {
        goto err;
    }
    msleep(50);
    in_args.arg = 0;
    status = alienware_wmax_command(&in_args, sizeof(in_args),
                                    WMAX_METHOD_FWUPDATE_GPIO, &out_data);
    if (!ACPI_SUCCESS(status)) {
        goto err;
    }
    msleep(50);
    in_args.arg = 257;
    status = alienware_wmax_command(&in_args, sizeof(in_args),
                                    WMAX_METHOD_FWUPDATE_GPIO, &out_data);
    if (ACPI_SUCCESS(status)) {
        if (out_data == 0) {
            return;
        }
    } else {
        goto err;
    }

    err:
    pr_err("alienware-wmi: unknown dfu status: %d\n", status);
}

static ssize_t mode_store(struct  kobject *kobj, struct kobj_attribute *attr,
        const char *buff, size_t count)
{
    int ret;
    
    ret = strncmp(buff, "dfu", 3);
    if (ret == 0)
        alienware_stm32_dfu();
    else
        alienware_stm32_run();

    return (count);
}

static struct kobj_attribute mode_attribute =
    __ATTR(mode, S_IRUGO | S_IWUSR, NULL, mode_store);

static struct attribute *attrs[] = {
    &mode_attribute.attr,
    NULL,
};

static struct attribute_group attr_group = {
    .attrs = attrs,
};

static struct kobject *kobj;

static int __init alienware_elc_dfu_init(void)
{
    int ret;

    kobj = kobject_create_and_add("alienware_elc_dfu", kernel_kobj);
    if (!kobj)
        return (-ENOMEM);
    ret = sysfs_create_group(kobj, &attr_group);
    if (ret)
        kobject_put(kobj);
    
    return (ret);
}

static void __exit alienware_elc_dfu_exit(void)
{
    if (kobj) {
        sysfs_remove_group(kobj, &attr_group);
        kobject_put(kobj);
    }
}

module_init(alienware_elc_dfu_init);
module_exit(alienware_elc_dfu_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Philippe Michaud-Boudreault <philmb3487@proton.me>");
MODULE_DESCRIPTION("Alienware ELC DFU trigger");
