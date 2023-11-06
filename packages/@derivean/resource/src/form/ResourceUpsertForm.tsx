import {ResourceIcon}         from "@derivean/ui";
import {
    Form,
    TextInput
}                             from "@use-pico/form";
import {type WithEntity}      from "@use-pico/types";
import {type FC}              from "react";
import {ResourceTypeSelect}   from "../input/ResourceTypeSelect";
import {withResourceMutation} from "../mutation/withResourceMutation";
import {type ResourceSchema}  from "../schema/ResourceSchema";
import {ResourceShapeSchema}  from "../schema/ResourceShapeSchema";

export namespace ResourceUpsertForm {
    export type Props =
        Form.PropsEx<
            withResourceMutation,
            ResourceShapeSchema,
            withResourceMutation["schema"]["request"],
            withResourceMutation["schema"]["response"]
        >
        & WithEntity.Schema.$<ResourceSchema>;
}

export const ResourceUpsertForm: FC<ResourceUpsertForm.Props> = (
    {
        entity,
        ...props
    }
) => {
    return <Form
        withMutation={withResourceMutation}
        schema={ResourceShapeSchema}
        inputs={{
            name:   TextInput,
            typeId: ResourceTypeSelect,
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
