import {
    Form,
    TextInput
}                             from "@use-pico/form";
import {type FC}              from "react";
import {withResourceMutation} from "../mutation/withResourceMutation";
import {ResourceShapeSchema}  from "../schema/ResourceShapeSchema";

export namespace ResourceUpsertForm {
    export type Props = Form.PropsEx<
        withResourceMutation,
        ResourceShapeSchema,
        withResourceMutation["schema"]["request"],
        withResourceMutation["schema"]["response"]
    >;
}

export const ResourceUpsertForm: FC<ResourceUpsertForm.Props> = props => {
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
        toRequest={values => ({
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
