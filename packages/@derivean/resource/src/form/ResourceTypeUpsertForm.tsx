import {ResourceIcon}             from "@derivean/ui";
import {
    Form,
    TextInput
}                                 from "@use-pico/form";
import {type WithEntity}          from "@use-pico/types";
import {type FC}                  from "react";
import {withResourceTypeMutation} from "../mutation/withResourceTypeMutation";
import {ResourceTypeSchema}       from "../schema/type/ResourceTypeSchema";
import {ResourceTypeShapeSchema}  from "../schema/type/ResourceTypeShapeSchema";

export namespace ResourceTypeUpsertForm {
    export type Props =
        Form.PropsEx<
            withResourceTypeMutation,
            ResourceTypeShapeSchema,
            withResourceTypeMutation["schema"]["request"],
            withResourceTypeMutation["schema"]["response"]
        >
        & WithEntity.Schema.$<ResourceTypeSchema>;
}

export const ResourceTypeUpsertForm: FC<ResourceTypeUpsertForm.Props> = (
    {
        entity,
        ...props
    }
) => {
    return <Form
        withTranslation={{
            namespace: "resource.type",
            label:     "upsert",
        }}
        withMutation={withResourceTypeMutation}
        schema={ResourceTypeShapeSchema}
        inputs={{
            name: TextInput,
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
