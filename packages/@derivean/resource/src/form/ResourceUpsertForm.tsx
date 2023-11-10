import {ResourceIcon}         from "@derivean/ui";
import {withDullSchema}       from "@use-pico/dull-stuff";
import {
    Form,
    TextInput
}                             from "@use-pico/form";
import {
    t,
    tx
}                             from "@use-pico/i18n";
import {type WithEntity}      from "@use-pico/types";
import {type FC}              from "react";
import {ResourceTypeSelect}   from "../input/ResourceTypeSelect";
import {withResourceMutation} from "../mutation/withResourceMutation";
import {ResourceSchema}       from "../schema/ResourceSchema";

export namespace ResourceUpsertForm {
    export type Props =
        Form.PropsEx<
            withResourceMutation,
            withDullSchema.Infer.ShapeSchema<ResourceSchema>,
            withResourceMutation["schema"]["request"],
            withResourceMutation["schema"]["response"]
        >
        & WithEntity.$<withDullSchema.Infer.Entity<ResourceSchema>>;
}

export const ResourceUpsertForm: FC<ResourceUpsertForm.Props> = (
    {
        entity,
        ...props
    }
) => {
    return <Form
        text={{
            submit:  entity ? t()`Update resource (label)` : t()`Create resource (label)`,
            success: {
                title:   t()`Success`,
                message: entity ? t()`Resource updated` : t()`Resource created`,
            }
        }}
        withMutation={withResourceMutation}
        schema={ResourceSchema.shape}
        inputs={{
            name:   props => <TextInput
                label={t()`Resource name`}
                placeholder={tx()`Resource name (placeholder)`}
                {...props}
            />,
            typeId: props => <ResourceTypeSelect
                text={{
                    label:       t()`Resource type`,
                    placeholder: tx()`Resource type (placeholder)`,
                    selector:    {
                        title: t()`Resource type selector`,
                    }
                }}
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
            name:   "",
            typeId: "",
        }}
        Render={({Input}) => <>
            <Input name={"name"}/>
            <Input name={"typeId"}/>
        </>}
        {...props}
    />;
};
