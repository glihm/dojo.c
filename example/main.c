#include "../dojo.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>

void on_entity_state_update(FieldElement key, CArrayModel models)
{
    printf("on_entity_state_update\n");
    printf("Key: 0x");
    for (size_t i = 0; i < 32; i++)
    {
        printf("%02x", key.data[i]);
    }
    printf("\n");

    for (size_t i = 0; i < models.data_len; i++)
    {
        printf("Model: %s\n", models.data[i].name);
        // for (size_t j = 0; j < models.data[i].children.data_len; j++)
        // {
        //     printf("Field: %s\n", models.data[i].children.data[j].name);
        //     printf("Value: %s\n", models.data[i].children.data[j].value);
        // }
    }
}

void hex_to_bytes(const char *hex, FieldElement* felt)
{

    if (hex[0] == '0' && hex[1] == 'x')
    {
        hex += 2;
    }

    // handle hex less than 64 characters - pad with 0
    size_t len = strlen(hex);
    if (len < 64)
    {
        char *padded = malloc(65);
        memset(padded, '0', 64 - len);
        padded[64 - len] = '\0';
        strcat(padded, hex);
        hex = padded;
    }

    for (size_t i = 0; i < 32; i++)
    {
        sscanf(hex + 2 * i, "%2hhx", &(*felt).data[i]);
    }
}

int main()
{
    const char *torii_url = "http://localhost:8080";
    const char *rpc_url = "http://localhost:5050";

    const char *player_key = "0x02038e0daba5c3948a6289e91e2a68dfc28e734a281c753933b8bd331e6d3dae";
    const char *player_address = "0x06162896d1d7ab204c7ccac6dd5f8e9e7c25ecd5ae4fcb4ad32e57786bb46e03";
    const char *player_signing_key = "0x1800000000300000180000000000030000000000003006001800006600";
    const char *world = "0x01385f25d20a724edc9c7b3bd9636c59af64cbaf9fcd12f33b3af96b2452f295";
    const char *actions = "0x03539c9b89b08095ba914653fb0f20e55d4b172a415beade611bc260b346d0f7";
    // Initialize world.data here...

    KeysClause entities[1] = {};
    // Initialize entities[0].model, entities[0].keys, and entities[0].keys_len here...
    entities[0].model = "Position";
    entities[0].keys.data = malloc(sizeof(char *));
    entities[0].keys.data_len = 1;
    entities[0].keys.data[0] = player_key;

    ResultToriiClient resClient = client_new(torii_url, rpc_url, "/ip4/127.0.0.1/tcp/9090", world, entities, 1);
    if (resClient.tag == ErrAccount)
    {
        printf("Failed to create client: %s\n", resClient.err.message);
        return 1;
    }
    ToriiClient *client = resClient.ok;


    // signing key
    FieldElement signing_key = {};
    hex_to_bytes(player_signing_key, &signing_key);

    // provider
    ResultProvider resProvider = provider_new(rpc_url);
    if (resProvider.tag == ErrProvider)
    {
        printf("Failed to create provider: %s\n", resProvider.err.message);
        return 1;
    }
    Provider *provider = resProvider.ok;

    // account
    ResultAccount resAccount = account_new(provider, signing_key, player_address);
    if (resAccount.tag == ErrAccount)
    {
        printf("Failed to create account: %s\n", resAccount.err.message);
        return 1;
    }
    Account *master_account = resAccount.ok;

    FieldElement master_address = account_address(master_account);
    printf("Master account: 0x");
    for (size_t i = 0; i < 32; i++)
    {
        printf("%02x", master_address.data[i]);
    }
    printf("\n");

    FieldElement burner_signer = signing_key_new();
    ResultAccount resBurner = account_deploy_burner(provider, master_account, burner_signer);
    if (resBurner.tag == ErrAccount)
    {
        printf("Failed to create burner: %s\n", resBurner.err.message);
        return 1;
    }

    Account *burner = resBurner.ok;

    printf("Burner account: 0x");
    FieldElement burner_address = account_address(burner);
    for (size_t i = 0; i < 32; i++)
    {
        printf("%02x", burner_address.data[i]);
    }
    printf("\n");

    ResultCOptionTy resTy = client_model(client, entities);
    if (resTy.tag == ErrCOptionTy)
    {
        printf("Failed to get entity: %s\n", resTy.err.message);
        return 1;
    }
    COptionTy ty = resTy.ok;

    if (ty.tag == SomeTy)
    {
        printf("Got entity\n");
        printf("Struct: %s\n", ty.some->struct_.name);
        for (size_t i = 0; i < ty.some->struct_.children.data_len; i++)
        {
            printf("Field: %s\n", ty.some->struct_.children.data[i].name);
        }

        ty_free(ty.some);
    }


    Query query = {};
    query.limit = 100;
    query.clause.tag = SomeClause;
    query.clause.some.tag = Keys;
    query.clause.some.keys.keys.data = malloc(sizeof(char *));
    query.clause.some.keys.keys.data_len = 1;
    query.clause.some.keys.keys.data[0] = player_address;
    query.clause.some.keys.model = "Moves";
    ResultCArrayEntity resEntities = client_entities(client, &query);
    if (resEntities.tag == ErrCArrayEntity)
    {
        printf("Failed to get entities: %s\n", resEntities.err.message);
        return 1;
    }

    CArrayEntity fetchedEntities = resEntities.ok;
    printf("Fetched %zu entities\n", fetchedEntities.data_len);
    for (size_t i = 0; i < fetchedEntities.data_len; i++)
    {
        // pritn hex of key
        printf("Key: 0x");
        for (size_t j = 0; j < 32; j++)
        {
            printf("%02x", fetchedEntities.data[i].hashed_keys.data[j]);
        }
        printf("\n");

        // print models name
        for (size_t j = 0; j < fetchedEntities.data[i].models.data_len; j++)
        {
            printf("Model: %s\n", fetchedEntities.data[i].models.data[j].name);
        }
    }

    // Result_bool resStartSub = client_start_subscription(client);
    // if (resStartSub.tag == Err_bool)
    // {
    //     printf("Failed to start subscription: %s\n", resStartSub.err.message);
    //     return 1;
    // }

    // Result_bool resAddEntities = client_add_models_to_sync(client, entities, 1);
    // if (resAddEntities.tag == Err_bool)
    // {
    //     printf("Failed to add entities to sync: %s\n", resAddEntities.err.message);
    //     return 1;
    // }

    // // print subscribed entities
    const CArrayKeysClause subscribed_models = client_subscribed_models(client);
    for (size_t i = 0; i < subscribed_models.data_len; i++)
    {
        printf("Subscribed entity: %s", subscribed_models.data[i].keys.data[0]);
        printf("\n");
    }

    FieldElement keys[1] = {};
    hex_to_bytes(player_key, &keys[0]);
    Resultbool resEntityUpdate = client_on_entity_state_update(client, keys, 0, &on_entity_state_update);
    if (resEntityUpdate.tag == Errbool)
    {
        printf("Failed to set entity update callback: %s\n", resEntityUpdate.err.message);
        return 1;
    }

    Call spawn = {
        .to = actions,
        .selector = "spawn",
    };

    Call move = {
        .to = actions,
        .selector = "move",
        .calldata = {
            .data = malloc(sizeof(FieldElement)),
            .data_len = 1,
        }};

    // move left felt(0x01)
    hex_to_bytes("0x01", &move.calldata.data[0]);

    BlockId block_id = {
        .tag = BlockTag_,
        .block_tag = Pending,
    };
    account_set_block_id(master_account, block_id);
    ResultFieldElement resSpawn = account_execute_raw(master_account, &spawn, 1);
    if (resSpawn.tag == Errbool)
    {
        printf("Failed to execute call: %s\n", resSpawn.err.message);
        return 1;
    }
    wait_for_transaction(provider, resSpawn.ok);

    printf("Spawned\n");

    ResultFieldElement resMove = account_execute_raw(master_account, &move, 1);
    if (resMove.tag == Errbool)
    {
        printf("Failed to execute call: %s\n", resMove.err.message);
        return 1;
    }
    wait_for_transaction(provider, resMove.ok);

    printf("Moved\n");

    // account_set_block_id(burner, block_id);
    // resSpawn = account_execute_raw(burner, &spawn, 1);
    // if (resSpawn.tag == Errbool)
    // {
    //     printf("Failed to execute call: %s\n", resSpawn.err.message);
    //     return 1;
    // }
    // wait_for_transaction(provider, resSpawn.ok);

    // printf("Spawned burner\n");

    // resMove = account_execute_raw(burner, &move, 1);
    // if (resMove.tag == Errbool)
    // {
    //     printf("Failed to execute call: %s\n", resMove.err.message);
    //     return 1;
    // }
    // wait_for_transaction(provider, resMove.ok);

    // printf("Moved burner\n");

    while (1)
    {

    }

    // Result_bool resRemoveEntities = client_remove_models_to_sync(client, entities, 1);
    // if (resRemoveEntities.tag == Err_bool)
    // {
    //     printf("Failed to remove entities to sync: %s\n", resRemoveEntities.err.message);
    //     return 1;
    // }

    // Remember to free the client when you're done with it.
    client_free(client);

    return 0;
}