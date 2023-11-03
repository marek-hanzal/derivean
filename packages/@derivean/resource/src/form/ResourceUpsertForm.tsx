import {
    Form,
    TextInput
}                                 from "@use-pico/form";
import {type WithEntity}          from "@use-pico/types";
import {type FC}                  from "react";
import {withResourceMutation}     from "../mutation/withResourceMutation";
import {ResourceRepositorySchema} from "../schema/ResourceRepositorySchema";
import {ResourceShapeSchema}      from "../schema/ResourceShapeSchema";

export namespace ResourceUpsertForm {
    export type Props =
        Form.PropsEx<
            withResourceMutation,
            ResourceShapeSchema,
            withResourceMutation["schema"]["request"],
            withResourceMutation["schema"]["response"]
        >
        & WithEntity.Schema.$<ResourceRepositorySchema>;
}

export const ResourceUpsertForm: FC<ResourceUpsertForm.Props> = (
    {
        entity,
        ...props
    }
) => {
    return <Form
        withTranslation={{
            namespace: "resource",
            label:     "upsert",
        }}
        withMutation={withResourceMutation}
        schema={ResourceShapeSchema}
        inputs={{
            name: TextInput,
        }}
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
