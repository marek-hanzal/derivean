import {ResourceIcon}             from "@derivean/ui";
import {withDullSchema}           from "@use-pico/dull-stuff";
import {
    Form,
    TextInput
}                                 from "@use-pico/form";
import {
    t,
    tx
}                                 from "@use-pico/i18n";
import {type WithEntity}          from "@use-pico/types";
import {type FC}                  from "react";
import {withResourceTypeMutation} from "../mutation/withResourceTypeMutation";
import {ResourceTypeSchema}       from "../schema/ResourceTypeSchema";

export namespace ResourceTypeUpsertForm {
    export type Props =
        Form.PropsEx<
            withResourceTypeMutation,
            withDullSchema.Infer.ShapeSchema<ResourceTypeSchema>,
            withResourceTypeMutation["schema"]["request"],
            withResourceTypeMutation["schema"]["response"]
        >
        & WithEntity.$<withDullSchema.Infer.Entity<ResourceTypeSchema>>;
}

export const ResourceTypeUpsertForm: FC<ResourceTypeUpsertForm.Props> = (
    {
        entity,
        ...props
    }
) => {
    return <Form
        text={{
            submit:  entity ? t()`Update resource type (label)` : t()`Create resource type (label)`,
            success: {
                title:   t()`Success`,
                message: entity ? t()`Resource type updated` : t()`Resource type created`,
            }
        }}
        withMutation={withResourceTypeMutation}
        schema={ResourceTypeSchema.shape}
        inputs={{
            name: props => <TextInput
                label={t()`Resource type name`}
                placeholder={tx()`Resource type name (placeholder)`}
                {...props}
            />,
        }}
        icon={<ResourceIcon/>}
        values={entity}
        toRequest={values => (entity ? {
            update: {
                update: values,
                query:  {
                    where: {
                        id: entity.id,
                    },
                },
            },
        } : {
            create: values,
        })}
        defaultValues={{
            name: "",
        }}
        Render={({Input}) => <>
            <Input name={"name"}/>
        </>}
        {...props}
    />;
};
